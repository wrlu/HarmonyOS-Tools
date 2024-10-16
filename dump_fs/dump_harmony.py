import os
import subprocess
import json

class Log:
    @staticmethod
    def send(msg):
        print('[Send] ' + msg)

    @staticmethod
    def print(msg):
        print(msg)

    @staticmethod
    def warn(msg):
        print('\033[0;33m' + msg + '\033[0m')

    @staticmethod
    def error(msg):
        print('\033[0;31m' + msg + '\033[0m')

def run_command(cmds, cwd='.'):
    return subprocess.Popen(cmds, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=cwd).communicate()[0]

def cmd_hdc_list_targets():
    return run_command(['hdc', 'list', 'targets'])

def cmd_bm_dump_all(connect_key):
    return run_command(['hdc', '-t', connect_key, 'shell', 'bm', 'dump', '-a'])

def cmd_bm_dump_bundle_info(connect_key, bundle_name):
    return run_command(['hdc', '-t', connect_key, 'shell', 'bm', 'dump', '-n', bundle_name])

def cmd_bm_copy_hap_to_tmp(connect_key, bundle_info):
    for hap_path in bundle_info['hap_paths']:
        run_command(['hdc', '-t', connect_key, 'shell', 'cp', hap_path, '/data/local/tmp/.dump_harmony_script/bundles/' + bundle_info['bundle_name'] + '/'])


def cmd_mkdir_dump_tmp(connect_key):
    run_command(['hdc', '-t', connect_key, 'shell', 'mkdir', '/data/local/tmp/.dump_harmony_script/'])

def cmd_mkdir_dump_tmp_subdir(connect_key, sub):
    run_command(['hdc', '-t', connect_key, 'shell', 'mkdir', '/data/local/tmp/.dump_harmony_script/' + sub])

def cmd_rm_dump_tmp(connect_key):
    run_command(['hdc', '-t', connect_key, 'shell', 'rm', '-rf', '"/data/local/tmp/.dump_harmony_script"'])

def dump_recv_bundles(connect_key):
    run_command(['hdc', '-t', connect_key, 'file', 'recv', '-a', '/data/local/tmp/.dump_harmony_script/bundles/', '.'])

def hdc_list_targets():
    output = cmd_hdc_list_targets().decode('ascii')
    lines = output.split('\n')
    return lines

def select_hdc_targets(pre_select_val):
    devices = hdc_list_targets()
    i = 1
    Log.print('Please select an hdc device:')
    for device in devices:
        if device == '':
            continue
        Log.print(str(i) + ' => ' + device)
        i = i + 1
    if pre_select_val == -1:
        select = int(input())
    else:
        select = pre_select_val
    Log.print('You select device on line '+str(select))
    if select > len(devices):
        Log.error('Out of range.')
        return None
    return devices[select - 1].strip()

def get_bundles(connect_key):
    bm_dump_output = cmd_bm_dump_all(connect_key).decode('ascii')
    lines = bm_dump_output.split('\n')
    bundles = []
    i = 0
    total = len(lines)
    for line in lines:
        if line.startswith('ID:') or line.strip() == '':
            i = i + 1
            continue
        line = line.strip().strip('\t')
        bundle_info = {}
        bundle_info['bundle_name'] = line
        bundle_info_output = cmd_bm_dump_bundle_info(connect_key, bundle_info['bundle_name']).decode('utf8')
        bundle_info_json = json.loads(bundle_info_output[bundle_info_output.index('{'):])
        bundle_info['hap_paths'] = []
        for hapModuleInfo in bundle_info_json['hapModuleInfos']:
            bundle_info['hap_paths'].append(hapModuleInfo['hapPath'])
        bundles.append(bundle_info)
        i = i + 1
        show_progress(i, total, 'Collect bundle info for ' + bundle_info['bundle_name'])
    return bundles

def show_progress(now, total, msg):
    if total == 0:
        return
    p = (now * 100) // total
    Log.print('[' + str(p) + '%' + '] ' + msg)

def main():
    connect_key = select_hdc_targets(-1)
    # Log.print('[Task 1] Dump HarmonyOS NEXT Apps')

    bundles = get_bundles(connect_key)
    os.makedirs('bundles', exist_ok=True)
    cmd_mkdir_dump_tmp(connect_key)
    cmd_mkdir_dump_tmp_subdir(connect_key, 'bundles')
    
    with open('bundle_index.csv', 'w') as f:
        f.write('bundle_name\n')
        i = 0
        total = len(bundles)
        for bundle in bundles:
            f.write(bundle['bundle_name']+'\n')
            f.flush()
            cmd_mkdir_dump_tmp_subdir(connect_key, 'bundles/' + bundle['bundle_name'])
            cmd_bm_copy_hap_to_tmp(connect_key, bundle)
            i = i + 1
            show_progress(i, total, 'Copy bundle binaries for ' + bundle['bundle_name'])

    Log.print('Recv bundle binaries...')
    dump_recv_bundles(connect_key)
    cmd_rm_dump_tmp(connect_key)
    Log.print('Done')

if __name__ == '__main__':
    main()