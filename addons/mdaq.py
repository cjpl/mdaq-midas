#!/usr/bin/env python
# -*- mode: python; coding: utf-8;
'''
   Python script to handle midas daq sessions.
   Author: Exaos Lee <Exaos DOT Lee AT gmail>
   Start Date: 2011-09-12, Chinese Fullmoon Festival
'''

import os, sys, signal, time, yaml
from pprint   import pprint
from optparse import OptionParser

if sys.hexversion > 0x03000000:
    from subprocess import getoutput
else:
    from commands import getoutput

#-----------------------------------------------------------------------
verbose = True

def find_execfile(execf, path=None):
    if os.path.isfile(execf): return execf
    else:
        if path is None: path = os.environ['PATH']
        for p in os.path.split(os.path.pathsep):
            f = os.path.join(p,execf)
            if os.path.isfile(f): return f
    return None

def dict_sign(dest, orig):
    if type(dest) != dict: return
    for k in list(dest.keys()):
        if k not in orig: continue
        if type(dest[k]) == dict: dict_sign(dest[k], orig[k])
        else: dest[k] = orig[k]

# Get list of PIDs of certain program
def get_pids(pname):
    return [int(k) for k in getoutput("pidof %s"%pname).strip().split()]

# Print program status
def get_status(pfile):
    pids = get_pids( os.path.basename(pfile) )
    if pids: return "PIDs = " + str(pids)
    return "Not running!"

# Kill all running program
def kill_prog(pname):
    print("Stopping %s ..."%pname)
    [os.kill(i,signal.SIGTERM) for i in get_pids(pname)]

#-----------------------------------------------------------------------

# MIDASSYS: midas
mdaq_cfg = find_execfile("mdaq_config")
mdaq_bin = "/usr/lib/daq-midas/bin"
mdaq_dir = "/usr/lib/daq-midas"
if mdaq_cfg:
    mdaq_bin = getoutput("%s --bindir"%(mdaq_cfg))
    mdaq_dir = getoutput("%s --execdir"%(mdaq_cfg))
mdaq_sys = {
    'path':mdaq_dir, 'bin':mdaq_bin, 'exptab':'',
    'host':'localhost',
    'mserver': { # MIDAS Server
        'enabled': True,  'options': ['-D', '-m'] },
    'mhttpd': {  # MIDAS HTTPD Server
        'enabled': True,  'options': ['-D',], 'port': 8080 },
    'mlogger': { # Default Data Logger
        'enabled': True,  'options': ['-D',] },
    'odbedit': { # OnlineDB Editor
        'enabled': False,  'options': [],
        'script':''},
    }

#-----------------------------------------------------------------------
# mdaq run
def mdaq_run(args):
    if not args: print("No midas program specified!"); return
    cmd = args[0]
    if not os.path.isfile(cmd): cmd = os.path.join(mdaq_bin, args[0])
    if os.path.isfile(cmd):
        opts = " ".join(args[1:])
        if verbose: print("Starting %s ...\n\tOptions: %s"%(cmd,opts))
        os.system( "%s %s"%(cmd,opts) )
    else:  print("%s:\n\texecutable doesn't exist!"%(cmd))

#-----------------------------------------------------------------------
# Experiment DAQ config
exp_fcnf = "mdaq.yaml"
exp_opts = ''

# mdaq session info
exp_conf = {
    'id': '',   # experiment ID
    'info': '', # experiment description
    'paths': { 'base': '.', 'data': './data', 'bin': './bin' },
    'frontend': { 'info': "Frontend code",
                  'name': 'frontend',
                  'options': ["-D",] },
    'analyzer': { 'info': "Online analyzer",
                  'name': 'analyzer',
                  'options': ["-D",] }
    }

def read_conf(fcnf):
    global mdaq_sys, exp_conf

    if not os.path.isfile(fcnf) and verbose:
        print("Configure file %s not found!"%fcnf)
        return

    cnf = yaml.load(open(fcnf))
    if not cnf and verbose:
        print("Read configure file (%s) FAILED!"%fcnf)
        return

    # Experiment info
    if 'experiment' in cnf: dict_sign( exp_conf, cnf['experiment'] )
    # MIDAS system
    if 'mdaq_sys' in cnf: dict_sign( mdaq_sys, cnf['mdaq_sys'] )

    # Experiment options
    global exp_opts
    daq_dir = os.path.abspath(exp_conf['paths']['base'])
    if exp_conf['id']:
        local_exptab = os.path.join(exp_conf['paths']['base'],'exptab')
        if os.path.isfile(local_exptab):
            os.environ.setdefault('MIDAS_EXPTAB', local_exptab)
            exp_opts = "-e %s"%(exp_conf['id'])
        else:
            print("## Failed to find ${MIDAS_EXPTAB} file!")
    if 'MIDAS_EXPTAB' not in os.environ and verbose:
        print("## Using environment:\n##\tMIDAS_DIR=%s"%(daq_dir))
        print("## In this mode, you cannot access DAQ remotely!\n")
        os.environ.setdefault("MIDAS_DIR", daq_dir)

    # Apply MDAQ_SYS environment
    if "MIDASSYS" not in os.environ:
        os.environ.setdefault("MIDASSYS", mdaq_sys['path'])

# Experiment session operations: info, start, stop, restart, status
def exp_info():
    if os.path.isfile(exp_fcnf):
        print("## Get settings from file:", exp_fcnf, "\n")
    else:
        print("## Using default experiment settings\n")
    print("==== Experiment information ====")
    pprint(exp_conf)
    print("\n==== MIDAS system info ====")
    pprint(mdaq_sys)

def exp_start():
    # Change dir
    daq_dir = os.path.abspath(exp_conf['paths']['base'])
    daq_bin = os.path.abspath(exp_conf['paths']['bin' ])
    daq_data= os.path.abspath(exp_conf['paths']['data'])
    old_dir = os.getcwd()
    os.chdir(daq_dir)

    # Midas server
    if not get_pids("mserver") and mdaq_sys['mserver']['enabled']:
        args = [os.path.join(mdaq_sys['bin'],'mserver'), ]
        args.extend(mdaq_sys['mserver']['options'])
        mdaq_run(args)
        time.sleep(1)

    # mhttpd
    if not get_pids('mhttpd') and mdaq_sys['mhttpd']['enabled']:
        args = [os.path.join(mdaq_sys['bin'],'mhttpd'), ]
        args.extend( ["-p %d"%(mdaq_sys['mhttpd']['port']), exp_opts] )
        args.extend( mdaq_sys['mhttpd']['options'] )
        mdaq_run(args)
        time.sleep(1)

    # Init OnlineDB
    if mdaq_sys['odbedit']['enabled'] and mdaq_sys['odbedit']['script']:
        print(("Initializing OnlineDB for experiment %s ..."%(exp_conf['id'])))
        args = [ os.path.join(mdaq_sys['bin'],'odbedit'), ]
        args.extend( ['-c @%s'%(mdaq_sys['odbedit']['script']), exp_opts] )
        args.extend( mdaq_sys['odbedit']['options'] )
        mdaq_run(args)
        time.sleep(1)

    # First stop, then start ...?

    # Frontend
    if exp_conf['frontend']['name']:
        args = [ os.path.join(daq_bin,exp_conf['frontend']['name']), exp_opts ]
        args.extend( exp_conf['frontend']['options'] )
        mdaq_run(args)
        time.sleep(2)
    
    # analyzer
    if exp_conf['analyzer']['name']:
        args = [ os.path.join(daq_bin,exp_conf['analyzer']['name']), exp_opts ]
        args.extend( exp_conf['analyzer']['options'] )
        mdaq_run(args)
        time.sleep(2)

    # Start Logger
    if not get_pids('mlogger') and mdaq_sys['mlogger']['enabled']:
        args = [ os.path.join(mdaq_sys['bin'],'mlogger'), exp_opts ]
        args.extend( mdaq_sys['mlogger']['options'] )
        mdaq_run(args)
        time.sleep(1)

    if verbose: print('''
## Finished starting sessions for experiment: %s
## Please point your web browser to http://localhost:%d/
## Or run: firefox http://localhost:%d/
## To look at live histograms, run: roody -Hlocalhost
'''%(exp_conf['id'], mdaq_sys['mhttpd']['port'], mdaq_sys['mhttpd']['port']))

    #--- end ---
    os.chdir(old_dir)

def exp_stop(totally=False):
    progs = [ exp_conf['analyzer']['name'], exp_conf['frontend']['name'] ]
    if totally: progs.extend( ['mlogger', 'mhttpd', 'mserver'] )
    for p in progs: kill_prog(p)

def exp_restart():
    exp_stop()
    exp_start()

def exp_status():
    # MIDAS services
    print("===== MIDAS services =====")
    for p in ['mserver', 'mhttpd', 'mlogger']:
        print("%s:"%p, get_status(p))
    # Exp info
    print("\n===== Experiment session =====")
    for p in ['analyzer', 'frontend']:
        print("%s (%s):"%(p,exp_conf[p]['name']),
              get_status(exp_conf[p]['name']))

# mdaq exp [info, start, stop, restart, status] [opt]
def mdaq_exp(args):
    parser = OptionParser(usage="mdaq [option] <cmd>", version="0.2")
    parser.add_option("-c","--config", metavar="config",
                      help="Read configuration from config")
    parser.add_option("-a","--all", action="store_true", dest="stop_all",
                      help="Stop all sessions")
    parser.add_option("-q","--quiet", action="store_true", dest="quiet",
                      help="Print more messages")
    cmds = ['info', 'start', 'stop', 'restart', 'status']

    (o,a) = parser.parse_args(args)
    if not a or len(a)>=2 or a[0] not in cmds: usage();  return

    global verbose
    if o.quiet: verbose = False

    global exp_fcnf
    if o.config: exp_fcnf = o.config
    read_conf(exp_fcnf)

    if o.stop_all and a[0] == "stop": exp_stop(True)
    else:   eval("exp_%s()"%(a[0]))

# run args
def run_args(args):
    opt_fcnf = ''
    if "-c" in args: opt_fcnf = "-c"
    elif "--config" in args: opt_fcnf = "--config"
    else: pass
    if opt_fcnf:
        fidx = args.index(opt_fcnf)+1
        if fidx< len(args):
            read_conf(args[fidx])
            args.remove(args[fidx])
            args.remove(opt_fcnf)
        else: usage(); return
    else: read_conf(exp_fcnf)
    mdaq_run(args)

#-----------------------------------------------------------------------
def usage():
    print(__doc__)
    print('''Usage: mdaq <command> [options]

Commands:
  run <prog> [opts]  ---  execute midas program, such as mserver, mstat, etc.
                          (e.g. "mdaq run mhttpd -p 8080")

  info    ---  display experiment daq information
  start   ---  start daq session
  stop    ---  stop daq session
  restart ---  restart daq session
  status  ---  display daq session status

Options:
  -c|--config <file>
               --- daq config file (default: mdaq.yaml)
  -a|--all     --- use with command "stop" to stop all midas sessions.
  -q|--quiet   --- quiet mode, display messages as little as possible.

''')

if __name__ == '__main__':
    if len(sys.argv)>1:
        if    sys.argv[1] == "run": run_args(sys.argv[2:])
        else: mdaq_exp(sys.argv[1:])
    else: usage(); exit()

