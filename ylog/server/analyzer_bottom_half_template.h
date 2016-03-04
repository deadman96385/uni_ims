/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

/**
 * Auto created by analyzer_bottom_half_template.sh
 * base on analyzer_bottom_half_template.py
 * Fri, 04 Mar 2016 11:50:06 +0800
 */

#define analyzer_bottom_half_template \
"import os\n" \
"import re\n" \
"import sys\n" \
"import optparse\n" \
"import shutil\n" \
"\n" \
"def merge_logs(files, output):\n" \
"    global cpath\n" \
"    with open(os.path.join(cpath, output), 'w') as alllog:\n" \
"        for f in files:\n" \
"            with open(os.path.join(cpath, f), 'r') as sublog:\n" \
"                shutil.copyfileobj(sublog, alllog)\n" \
"\n" \
"def split_log(infiles, logdict):\n" \
"    global cpath\n" \
"    fddict = {}\n" \
"    keys = logdict.keys()\n" \
"    for key in keys:\n" \
"        fddict[key] = open(os.path.join(cpath, logdict[key]), 'w')\n" \
"\n" \
"    for eachfile in infiles:\n" \
"        with open(os.path.join(cpath, eachfile), 'r') as f:\n" \
"            for line in f.readlines():\n" \
"                if line[0:id_token_len] in keys:\n" \
"                    fddict[line[0:id_token_len]].write(line[id_token_len:])\n" \
"\n" \
"    for key in keys:\n" \
"        fddict[key].close()\n" \
"\n" \
"def main():\n" \
"    global cpath\n" \
"    cpath = os.path.dirname(os.path.abspath(sys.argv[0]))\n" \
"    parser = optparse.OptionParser()\n" \
"    parser.add_option('-r', dest='remove', default=False, action='store_true', help='remove the original log files')\n" \
"    parser.add_option('-m', dest='merge', default=False, action='store_true', help='merge all the log files')\n" \
"    options, args = parser.parse_args()\n" \
"\n" \
"    if not args:\n" \
"        allfiles = os.listdir(os.path.join(cpath, logpath))\n" \
"        pat = re.compile('.*\\.?[0-9]+$')\n" \
"        logfilenames = [f for f in allfiles if pat.match(f)]\n" \
"    else:\n" \
"        logfilenames = args;\n" \
"\n" \
"    logfilenames.sort(reverse = True)\n" \
"\n" \
"    if options.merge or not logdict :\n" \
"        merge_logs(logfilenames, merged)\n" \
"    else :\n" \
"        split_log(logfilenames, logdict)\n" \
"    if options.remove:\n" \
"        for log in logfilenames:\n" \
"            os.remove(os.path.join(cpath, log))\n" \
"        os.remove(sys.argv[0])\n" \
"\n" \
"if __name__ == '__main__':\n" \
"    main()\n" \
""
