import os
import re
import sys
import optparse
import shutil

def merge_logs(files, output):
    global cpath
    with open(os.path.join(cpath, output), 'w') as alllog:
        for f in files:
            with open(os.path.join(cpath, f), 'r') as sublog:
                shutil.copyfileobj(sublog, alllog)

def split_log(infiles, logdict):
    global cpath
    fddict = {}
    keys = logdict.keys()
    for key in keys:
        fddict[key] = open(os.path.join(cpath, logdict[key]), 'w')

    for eachfile in infiles:
        with open(os.path.join(cpath, eachfile), 'r') as f:
            for line in f.readlines():
                if line[0:id_token_len] in keys:
                    fddict[line[0:id_token_len]].write(line[id_token_len:])

    for key in keys:
        fddict[key].close()

def main():
    global cpath
    cpath = os.path.dirname(os.path.abspath(sys.argv[0]))
    parser = optparse.OptionParser()
    parser.add_option('-r', dest='remove', default=False, action='store_true', help='remove the original log files')
    parser.add_option('-m', dest='merge', default=False, action='store_true', help='merge all the log files')
    options, args = parser.parse_args()

    if not args:
        allfiles = os.listdir(os.path.join(cpath, logpath))
        pat = re.compile('.*\.?[0-9]+$')
        logfilenames = [f for f in allfiles if pat.match(f)]
    else:
        logfilenames = args;

    logfilenames.sort(reverse = True)

    if options.merge or not logdict :
        merge_logs(logfilenames, merged)
    else :
        split_log(logfilenames, logdict)
    if options.remove:
        for log in logfilenames:
            os.remove(os.path.join(cpath, log))
        os.remove(sys.argv[0])

if __name__ == '__main__':
    main()
