#!/usr/bin/env python3
"""Perfil de boot do MRBE no QEMU: initcalls do kernel, console e memoria.

A primeira inicializacao usa initcall_debug e faz login pelo console serial para
ler o dmesg; a segunda, sem instrumentacao, le /proc/meminfo e ps com o launcher
na tela. As demais (--runs) medem so o tempo ate MRBE_BOOT_OK. Tempos sob TCG nao sao tempos do hardware real.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import re
import select
import signal
import statistics
import subprocess
import sys
import time

APPEND='root=/dev/vda rootwait console=ttyAMA0 rw loglevel=4'
MARKER=b'MRBE_BOOT_OK'

def parse_initcalls(dmesg: str) -> list[tuple[int,str]]:
    """(usecs, funcao) de cada initcall, do mais lento para o mais rapido."""
    calls=[(int(us),fn.split('+')[0]) for fn,us in
           re.findall(r'initcall (\S+) returned -?\d+ after (\d+) usecs',dmesg)]
    return sorted(calls,reverse=True)

def parse_event(dmesg: str, text: str) -> float|None:
    """Segundos desde o inicio do kernel em que a mensagem aparece (printk.time)."""
    m=re.search(r'^\[\s*(\d+\.\d+)\] '+re.escape(text),dmesg,re.M)
    return float(m.group(1)) if m else None

def parse_kernel_memory(dmesg: str) -> dict[str,int]:
    """KiB por secao do kernel, da linha 'Memory:' do boot."""
    m=re.search(r'Memory: \d+K/\d+K available \(([^)]*)\)',dmesg)
    if not m: return {}
    return {name.replace('-','_'):int(kb) for kb,name in re.findall(r'(\d+)K ([\w-]+)',m.group(1))}

def parse_meminfo(text: str) -> dict[str,int]:
    return {k:int(v) for k,v in re.findall(r'^(\w+):\s+(\d+) kB',text,re.M)}

def parse_ps(text: str) -> list[tuple[int,str]]:
    """(RSS KiB, comando) dos processos de usuario; threads do kernel tem RSS 0."""
    procs=[]
    for line in text.splitlines()[1:]:
        f=line.split(None,2)
        if len(f)==3 and f[1].isdigit() and int(f[1])>0:
            procs.append((int(f[1]),f[2]))
    return procs

def console_gaps(lines: list[tuple[float,str]], limit: int=8) -> list[tuple[float,float,str]]:
    """Maiores intervalos entre linhas do console: (inicio, duracao, linha anterior)."""
    gaps=[(a[0],b[0]-a[0],a[1]) for a,b in zip(lines,lines[1:])]
    return sorted(gaps,key=lambda g:g[1],reverse=True)[:limit]

class Guest:
    """QEMU com virtio-gpu, teclado e mouse; console serial em stdin/stdout."""
    def __init__(self, qemu: str, images: Path, extra: str=''):
        cmd=[qemu,'-M','virt','-cpu','cortex-a53','-smp','2','-m','1024','-accel','tcg',
             '-kernel',str(images/'Image'),'-append',f'{APPEND} {extra}'.strip(),
             '-drive',f'file={images/"rootfs.ext4"},if=none,format=raw,id=hd0,snapshot=on',
             '-device','virtio-blk-device,drive=hd0',
             '-netdev','user,id=net0','-device','virtio-net-device,netdev=net0',
             '-device','virtio-gpu-pci','-device','virtio-keyboard-pci','-device','virtio-mouse-pci',
             '-display','none','-serial','stdio','-monitor','none']
        self.started=time.monotonic()
        self.proc=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT,start_new_session=True,bufsize=0)
        self.raw=b''; self.pending=b''; self.cursor=0
        self.lines: list[tuple[float,str]]=[]  # (inicio da linha, texto), relogio do host
        self.line_start=0.0
        self.marker_at: float|None=None
    def _pump(self, seconds: float) -> None:
        assert self.proc.stdout is not None
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            readable,_,_=select.select([self.proc.stdout],[],[],0.05)
            if not readable: continue
            chunk=os.read(self.proc.stdout.fileno(),65536)
            if not chunk: raise RuntimeError('QEMU encerrou antes do fim do perfil')
            now=time.monotonic()-self.started
            if self.marker_at is None and MARKER in self.raw[-len(MARKER):]+chunk:
                self.marker_at=now
            self.raw+=chunk
            for piece in re.split(rb'(?<=\n)',chunk):
                if not piece: continue
                if not self.pending: self.line_start=now
                self.pending+=piece
                if piece.endswith(b'\n'):
                    text=self.pending.decode(errors='replace').strip()
                    if text: self.lines.append((self.line_start,text))
                    self.pending=b''
    def wait_for(self, pattern: bytes, timeout: float) -> None:
        """Espera o padrao no console, a partir do fim da ultima espera."""
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            m=re.search(pattern,self.raw[self.cursor:])
            if m:
                self.cursor+=m.end(); return
            self._pump(0.05)
        raise TimeoutError(pattern.decode())
    def shell(self, command: str, timeout: float=60) -> str:
        """Executa no shell serial e devolve a saida, delimitada por marcas unicas."""
        assert self.proc.stdin is not None
        tag=f'@@{time.monotonic_ns()}'
        start=len(self.raw)
        self.proc.stdin.write(f'echo {tag}B; {command}; echo {tag}E\n'.encode())
        pattern=re.compile(re.escape(f'{tag}B'.encode())+rb'\r?\n(.*?)'+re.escape(f'{tag}E'.encode()),re.S)
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            m=pattern.search(self.raw[start:])
            if m: return m.group(1).decode(errors='replace').replace('\r','')
            self._pump(0.1)
        raise TimeoutError(command)
    def close(self) -> None:
        if self.proc.poll() is None:
            try:
                os.killpg(self.proc.pid,signal.SIGKILL)
            except ProcessLookupError:
                pass
            self.proc.wait()

def login(guest: Guest, timeout: float) -> None:
    guest.wait_for(MARKER,timeout)
    guest.wait_for(rb'login:',30)
    guest.proc.stdin.write(b'root\n')  # type: ignore[union-attr]
    guest._pump(3)  # tempo para o launcher abrir a tela

def profile(qemu: str, images: Path, timeout: float, log_path: Path) -> dict:
    """Boot instrumentado: initcalls, eventos do dmesg e intervalos do console."""
    guest=Guest(qemu,images,'initcall_debug printk.time=1 log_buf_len=1M')
    try:
        login(guest,timeout)
        dmesg=guest.shell('dmesg',120)
        launcher_log=guest.shell('cat /var/log/mrbe-launcher.log 2>/dev/null')
    finally:
        guest.close()
        log_path.write_bytes(guest.raw)
    calls=parse_initcalls(dmesg)
    assert guest.marker_at is not None
    userspace=[(t,l) for t,l in guest.lines if t<=guest.marker_at]
    return {
        'kernel_init_sec':parse_event(dmesg,'Run /sbin/init'),
        'launcher_first_frame_sec':parse_event(dmesg,'mrbe-launcher: primeira tela'),
        'initcalls':{'count':len(calls),'total_ms':round(sum(us for us,_ in calls)/1000,1),
                     'slowest':[{'usecs':us,'function':fn} for us,fn in calls[:10]]},
        'kernel_memory_kib':parse_kernel_memory(dmesg),
        'console_gaps':[{'at_sec':round(a,3),'sec':round(d,3),'after':l} for a,d,l in console_gaps(userspace)],
        'launcher_log':launcher_log.strip().splitlines(),
    }

def memory(qemu: str, images: Path, timeout: float) -> dict:
    """Boot sem instrumentacao: memoria e processos com o launcher na tela."""
    guest=Guest(qemu,images)
    try:
        login(guest,timeout)
        mem=parse_meminfo(guest.shell('cat /proc/meminfo'))
        # O shell do login e o proprio ps sao da medicao, nao do sistema.
        procs=[p for p in parse_ps(guest.shell('ps -o pid,rss,args'))
               if p[1]!='-sh' and not p[1].startswith('ps ')]
    finally:
        guest.close()
    return {'memory_kib':{k:mem.get(k) for k in ('MemTotal','MemAvailable','MemFree')},
            'processes':{'count':len(procs),'rss_total_kib':sum(r for r,_ in procs),
                         'list':[{'rss_kib':r,'command':c} for r,c in sorted(procs,reverse=True)]}}

def timed_boot(qemu: str, images: Path, timeout: float) -> float:
    guest=Guest(qemu,images)
    try:
        guest.wait_for(MARKER,timeout)
        assert guest.marker_at is not None
        return guest.marker_at
    finally:
        guest.close()

def report(r: dict) -> str:
    out=['Perfil de boot MRBE (QEMU TCG; relogio do host salvo indicacao)']
    runs=r['boot_ok_runs_sec']
    if runs:
        out.append(f'  MRBE_BOOT_OK: mediana {statistics.median(runs):.2f}s em {len(runs)} boots '
                   f'(min {min(runs):.2f}s, max {max(runs):.2f}s)')
    p=r['profile']
    if p['kernel_init_sec'] is not None:
        out.append(f'  Kernel ate /sbin/init (relogio do guest): {p["kernel_init_sec"]:.3f}s')
    if p['launcher_first_frame_sec'] is not None:
        out.append(f'  Primeira tela do launcher (relogio do guest): {p["launcher_first_frame_sec"]:.3f}s')
    ic=p['initcalls']
    out.append(f'  Initcalls: {ic["count"]}, somando {ic["total_ms"]} ms; mais lentas:')
    out+=[f'    {c["usecs"]/1000:7.1f} ms  {c["function"]}' for c in ic['slowest']]
    out.append('  Maiores intervalos no console antes de MRBE_BOOT_OK:')
    out+=[f'    {g["sec"]:6.3f}s em {g["at_sec"]:6.3f}s, depois de: {g["after"][:60]}' for g in p['console_gaps']]
    m=r['memory']['memory_kib']
    if m['MemTotal']:
        out.append(f'  Memoria: {m["MemTotal"]//1024} MiB visiveis, {m["MemAvailable"]//1024} MiB disponiveis')
    pr=r['memory']['processes']
    out.append(f'  Processos de usuario: {pr["count"]}, RSS somado {pr["rss_total_kib"]/1024:.1f} MiB')
    out+=[f'    {x["rss_kib"]:6d} KiB  {x["command"][:60]}' for x in pr['list'][:6]]
    return '\n'.join(out)

def main() -> int:
    parser=argparse.ArgumentParser()
    parser.add_argument('--qemu', default=os.environ.get('QEMU','qemu-system-aarch64'))
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--images', type=Path, help='padrao: ROOT/work/output/images')
    parser.add_argument('--runs', type=int, default=int(os.environ.get('PROFILE_RUNS','5')))
    parser.add_argument('--timeout', type=int, default=int(os.environ.get('SMOKE_TIMEOUT','150')))
    args=parser.parse_args()
    images=args.images or args.root/'work/output/images'
    for f in (images/'Image',images/'rootfs.ext4'):
        if not f.is_file() or f.stat().st_size==0:
            print(f'[BLOQUEADO] Imagem ausente: {f}. Rode: make build',file=sys.stderr)
            return 2
    if args.runs<0 or args.timeout<=0:
        print('[ERRO] --runs e --timeout invalidos',file=sys.stderr); return 2
    log_dir=args.root/'logs/testes'; log_dir.mkdir(parents=True,exist_ok=True)
    try:
        result={'profile':profile(args.qemu,images,args.timeout,log_dir/'boot-profile-latest.log'),
                'memory':memory(args.qemu,images,args.timeout),
                'boot_ok_runs_sec':[round(timed_boot(args.qemu,images,args.timeout),3)
                                    for _ in range(args.runs)]}
    except (FileNotFoundError,PermissionError) as e:
        print(f'[ERRO] QEMU indisponivel: {e}',file=sys.stderr); return 2
    except (TimeoutError,RuntimeError,BrokenPipeError) as e:
        print(f'[FALHA] Boot nao concluido: {e!r}. Log do boot instrumentado: {log_dir/"boot-profile-latest.log"}',file=sys.stderr)
        return 1
    (log_dir/'boot-profile-latest.json').write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n')
    print(report(result))
    print(f'Relatorio completo: {log_dir/"boot-profile-latest.json"}')
    return 0

if __name__=='__main__': sys.exit(main())
