"""Native Tarot Reader interaction tests; isolated persistent profiles, no physical watches.

Run with the installed pebble-tool Python after pebble build.
"""
import argparse
import datetime
import hashlib
import json
import re
import os
from pathlib import Path
import time
import tempfile
import subprocess
from uuid import UUID
from types import SimpleNamespace

SDK=Path.home()/'Library/Application Support/Pebble SDK/SDKs/4.33.1'
toolchain=str(SDK.resolve()/'toolchain/bin')
os.environ.setdefault('PEBBLE_QEMU_PATH',toolchain+'/qemu-pebble')
os.environ['PATH']=toolchain+os.pathsep+os.environ['PATH']
import png
from pebble_tool.commands.screenshot import ScreenshotCommand
from pebble_tool.commands.install import ToolAppInstaller
from libpebble2.protocol.apps import AppRunState,AppRunStateStart,AppRunStateStop
from libpebble2.protocol.logs import AppLogMessage,AppLogShippingControl
import pebble_tool.sdk.emulator as emulator
from libpebble2.communication.transports.qemu.protocol import QemuButton
from pebble_tool.commands.emucontrol import send_data_to_qemu

ROOT=Path(__file__).resolve().parent.parent
PBW=ROOT/'build'/f'{ROOT.name}.pbw' # The SDK uses the checkout folder's name.
APP=UUID(json.loads((ROOT/'package.json').read_text())['pebble']['uuid'])

def run(platform,fresh=False):
    installed_sha=hashlib.sha256(PBW.read_bytes()).hexdigest()
    out=ROOT/'build/evidence';out.mkdir(exist_ok=True)
    state=Path(tempfile.mkdtemp(prefix='emulator-fresh-',dir=ROOT/'build')) if fresh else ROOT/'build/emulator-state'
    state.mkdir(exist_ok=True)
    def persist(target,version=None):
        p=state/target;p.mkdir(exist_ok=True);return str(p)
    emulator.get_sdk_persist_dir=persist
    emulator.get_emulator_info_path=lambda:str(state/'emulators.json')
    emulator.get_default_account=lambda:SimpleNamespace(is_logged_in=False)
    bridge_log=(out/f'{platform}-bridge.log').open('w')
    emulator.ManagedEmulatorTransport._get_output=lambda self:bridge_log
    cmd=ScreenshotCommand();cmd._set_debugging(0)
    def shutdown():
        info=emulator.get_emulator_info(platform,'4.33.1')
        if info:
            for key in ('qemu','pypkjs','websockify'):
                pid=info.get(key,{}).get('pid')
                if not pid:continue
                process=subprocess.run(['ps','-p',str(pid),'-o','command='],capture_output=True,text=True)
                if process.returncode==0 and str(state/platform) not in process.stdout:
                    raise RuntimeError(f'Refusing to stop PID {pid}: outside this test profile')
        cmd._shutdown_platform_emulator(platform,'4.33.1')
    shutdown()
    try:
        watch=cmd._connect_emulator(platform,'4.33.1');cmd.pebble=watch
    except BaseException:
        shutdown();bridge_log.close();raise
    logs=[];frames=[]
    def log(packet):logs.append(str(packet.message));print(str(packet.message),flush=True)
    handle=watch.register_endpoint(AppLogMessage,log)
    watch.send_packet(AppLogShippingControl(enable=True))
    args=argparse.Namespace(no_correction=True,scale=1,no_open=True,v=0)
    def grab(name):
        pixels=cmd._grab_processed_image(args,show_progress=False)
        file=out/f'{platform}-{name}.png';png.from_array(pixels,mode='RGBA;8').save(str(file));frames.append(str(file.relative_to(ROOT)));return pixels
    def current():
        for line in reversed(logs):
            if 'Tarot screen=' in line:
                return {k:int(v) for k,v in re.findall(r'(screen|menu|count|selected|revealed|card|reverse)=(\d+)',line)}
        raise AssertionError('No native state log')
    def button(name):
        previous=sum('Tarot screen=' in line for line in logs)
        send_data_to_qemu(watch.transport,QemuButton(state=getattr(QemuButton.Button,name)))
        time.sleep(.08);send_data_to_qemu(watch.transport,QemuButton(state=0))
        deadline=time.monotonic()+4
        if previous:
            while sum('Tarot screen=' in line for line in logs)==previous and time.monotonic()<deadline:time.sleep(.03)
            assert sum('Tarot screen=' in line for line in logs)>previous, 'Button received no native response: '+name
        time.sleep(.25)
    def expect(**values):
        actual=current()
        assert all(actual[k]==v for k,v in values.items()),(values,actual)
    try:
        time.sleep(4);button('Back')
        ToolAppInstaller(watch,str(PBW),quiet=True).install()
        watch.send_packet(AppRunState(data=AppRunStateStart(uuid=APP)));time.sleep(1)
        expect(screen=0,menu=1);grab('menu')
        button('Down');button('Down');expect(menu=3)
        button('Select');expect(screen=1,count=10,revealed=0);grab('celtic-cross')
        button('Select');expect(screen=2);grab('card-back')
        button('Select');expect(screen=2,revealed=1);card=current();hero=grab('card')
        time.sleep(.7);assert grab('card-idle')==hero,'Idle card changed'
        button('Select');expect(screen=3);before=grab('meaning')
        for _ in range(3):button('Down')
        assert grab('meaning-scrolled')!=before,'Meaning does not scroll'
        button('Back');expect(screen=2);assert grab('card-return')==hero
        button('Down');expect(selected=1,screen=2,revealed=1)
        button('Select');expect(revealed=3);grab('second-card')
        button('Back');expect(screen=1);button('Back');expect(screen=0)
        watch.send_packet(AppRunState(data=AppRunStateStop(uuid=APP)));time.sleep(.5)
        watch.send_packet(AppRunState(data=AppRunStateStart(uuid=APP)));time.sleep(.8)
        expect(screen=0,menu=0,selected=1,revealed=3)
        button('Select');expect(screen=1);button('Up');expect(selected=0,card=card['card'],reverse=card['reverse'])
        grab('resumed-spread');button('Back')
        for _ in range(7):button('Down')
        expect(menu=7);button('Select');grab('reversals-off')
        for _ in range(3):button('Up')
        expect(menu=4);button('Select');expect(screen=1,count=5);grab('open-table')
        for i in range(5):
            button('Select');expect(screen=2,reverse=0);button('Select');button('Back');button('Down')
        expect(revealed=31);grab('open-table-revealed')
        button('Back');button('Down');expect(menu=1);button('Select');expect(count=1,screen=1)
        button('Back');button('Down');button('Down');expect(menu=2);button('Select');expect(count=3,screen=1);grab('three-cards')
        assert hashlib.sha256(PBW.read_bytes()).hexdigest()==installed_sha
        assert not any(any(term in l.lower() for term in ['crash','fault','could not','unavailable']) for l in logs)
        report={'platform':platform,'pbwSHA256':installed_sha,'capturedAt':datetime.datetime.now(datetime.timezone.utc).isoformat(),'passed':True,'checks':['Celtic Cross','reveal','meaning scroll','Back navigation','static idle','reading persistence','reversals off','open table','one card','three cards'],'frames':frames,'logs':logs,'limits':'Native emulator only. Physical-watch readability remains untested.'}
        (out/f'{platform}-report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',platform,flush=True)
    finally:
        watch.unregister_endpoint(handle);cmd._close_pebble_connection(watch);cmd.pebble=None;shutdown();bridge_log.close()

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('platform');parser.add_argument('--fresh',action='store_true');args=parser.parse_args();run(args.platform,args.fresh)
