/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

/**
 * Auto created by analyzer_bottom_half_template.sh
 * base on analyzer_bottom_half_template.py
 * Wed, 11 May 2016 18:04:23 +0800
 */

#define analyzer_bottom_half_template \
"import os\n" \
"import re\n" \
"import sys\n" \
"import optparse\n" \
"import shutil\n" \
"import struct\n" \
"\n" \
"# Python version check\n" \
"ver = sys.version_info\n" \
"\n" \
"if ver[0] == 3:\n" \
"    mopen = open;\n" \
"else:\n" \
"    def mopen(file, mode='rb', buffering=-1, encoding=None, errors=None, newline=None, closefd=True, opener=None):\n" \
"        return open(file, mode+'b')\n" \
"\n" \
"class ytag_fd(object):\n" \
"    pass\n" \
"\n" \
"def ytag_extract_file_ytfd(file_name):\n" \
"    if file_name not in ytfd_file:\n" \
"        file_name_entry = ytag_fd() # create a new class object\n" \
"        file_name_entry.ytfds = {}\n" \
"        file_name_entry.name = os.path.join(ytag_folder_abspath, file_name)\n" \
"        file_name_entry.count = 0\n" \
"        ytfd_file[file_name] = file_name_entry\n" \
"    else:\n" \
"        ytfd_file[file_name].count = ytfd_file[file_name].count + 1\n" \
"    count = ytfd_file[file_name].count\n" \
"    ytfd = ytag_fd() # create a new class object\n" \
"    ytfd.name = os.path.join(ytag_folder_abspath, '%04d.%s' % (count, file_name))\n" \
"    ytfd.fd = open(ytfd.name, 'wb')\n" \
"    ytfd.size = 0\n" \
"    ytfd_file[file_name].ytfds[count] = ytfd\n" \
"    return ytfd\n" \
"\n" \
"def ytag_extract_file(fd_src, ytfd):\n" \
"    while True:\n" \
"        ytag = fd_src.read(8)\n" \
"        if len(ytag) == 8:\n" \
"            ytags = struct.unpack('<II', ytag)\n" \
"            tag = ytags[0]\n" \
"            dlen = ytags[1]\n" \
"            if dlen < YTAG_STRUCT_SIZE:\n" \
"                print('Fatal dlen = %d at pos %d' % (dlen, fd_src.tell()))\n" \
"                return -1\n" \
"            dlen = dlen - YTAG_STRUCT_SIZE\n" \
"            if tag == YTAG_TAG_RAWDATA: # file data\n" \
"                if dlen == 0:\n" \
"                    continue\n" \
"                rdata = fd_src.read(dlen)\n" \
"                ytfd.fd.write(rdata)\n" \
"                rdlen = len(rdata)\n" \
"                ytfd.size = ytfd.size + rdlen\n" \
"                if rdlen != dlen:\n" \
"                    print('Fatal rdlen = %d, but dlen = %d at pos %d' % (rdlen, dlen, fd_src.tell()))\n" \
"                    return -1\n" \
"            elif tag == YTAG_TAG_NEWFILE_BEGIN: # begin of a new file\n" \
"                if dlen == 0:\n" \
"                    new_file_name = 'ytag_' + merged\n" \
"                else:\n" \
"                    new_file_name = fd_src.read(dlen).decode('utf-8')\n" \
"                    # python3 use decode() to remove \"b''\", 'utf-8' let us can decode chinese character\n" \
"                if ytag_extract_file(fd_src, ytag_extract_file_ytfd(new_file_name)) < 0:\n" \
"                    return -1\n" \
"            elif tag == YTAG_TAG_NEWFILE_END or tag == YTAG_MAGIC: # end of a file or ytag file magic\n" \
"                if dlen > 0:\n" \
"                    rdata = fd_src.read(dlen)\n" \
"                if tag == YTAG_TAG_NEWFILE_END:\n" \
"                    ytfd.fd.close()\n" \
"                    return 0\n" \
"            else:\n" \
"                print('Fatal tag = 0x%08x at pos %d does not support' % (tag, fd_src.tell()))\n" \
"                return -1\n" \
"        elif len(ytag) == 0:\n" \
"            return 0\n" \
"        else:\n" \
"            print('Fatal len(ytag) = %d at pos %d' % (len(ytag), fd_src.tell()))\n" \
"            return -1\n" \
"\n" \
"def ytag_parse(ytag_logfile):\n" \
"    global ytfd_file\n" \
"    ytfd_file = {}\n" \
"    sys.setrecursionlimit(90000) # otherwise will meet \"RuntimeError: maximum recursion depth exceeded in cmp\"\n" \
"    with open(os.path.join(analyzer_relative_path, ytag_logfile), 'rb') as fd_src:\n" \
"        ytag_extract_file(fd_src, ytag_extract_file_ytfd(merged))\n" \
"    for file_name in ytfd_file:\n" \
"        ytfds = ytfd_file[file_name].ytfds\n" \
"        for ytfd_index in ytfds:\n" \
"            ytfd = ytfds[ytfd_index]\n" \
"            ytfd.fd.close() # must close, otherwise can't work in windows python\n" \
"            if ytfd.size == 0:\n" \
"                os.remove(ytfd.name);\n" \
"                ytfd_file[file_name].count = ytfd_file[file_name].count - 1\n" \
"            else:\n" \
"                ytfd_latest = ytfd\n" \
"        if ytfd_file[file_name].count == 0:\n" \
"            os.rename(ytfd_latest.name, ytfd_file[file_name].name)\n" \
"\n" \
"def merge_logs(files, output):\n" \
"    with open(output, 'wb') as alllog:\n" \
"        for f in files:\n" \
"            with open(os.path.join(analyzer_relative_path, f), 'rb') as sublog:\n" \
"                shutil.copyfileobj(sublog, alllog)\n" \
"\n" \
"def split_log(infiles, logdict):\n" \
"    fddict = {}\n" \
"    keys = logdict.keys()\n" \
"    for key in keys:\n" \
"        fddict[key] = mopen(os.path.join(analyzer_relative_path, logdict[key]), 'w', encoding='utf8', errors='replace')\n" \
"\n" \
"    for eachfile in infiles:\n" \
"        with mopen(os.path.join(analyzer_relative_path, eachfile), 'r', encoding='utf8', errors='replace') as f:\n" \
"            for line in f.readlines():\n" \
"                if line[0:id_token_len] in keys:\n" \
"                    fddict[line[0:id_token_len]].write(line[id_token_len:])\n" \
"\n" \
"    for key in keys:\n" \
"        fddict[key].close()\n" \
"\n" \
"def main():\n" \
"    global analyzer_relative_path\n" \
"    global ytag_folder_abspath\n" \
"    os.chdir(os.path.dirname(os.path.abspath(sys.argv[0])))\n" \
"    analyzer_relative_path = '.'+os.sep\n" \
"    parser = optparse.OptionParser()\n" \
"    parser.add_option('-r', dest='remove', default=False, action='store_true', help='remove the original log files')\n" \
"    parser.add_option('-m', dest='merge', default=False, action='store_true', help='merge all the log files')\n" \
"    parser.add_option('-R', dest='reverse', default=True, action='store_false', help='sort the files reverse')\n" \
"    parser.add_option('-d', dest='debug', default=False, action='store_true', help='output some debug info')\n" \
"    options, args = parser.parse_args()\n" \
"\n" \
"    if not args:\n" \
"        allfiles = os.listdir(os.path.join(analyzer_relative_path, logpath))\n" \
"        pat = re.compile('.*\\.?[0-9]+$')\n" \
"        logfilenames = [f for f in allfiles if pat.match(f)]\n" \
"    else:\n" \
"        logfilenames = args;\n" \
"\n" \
"    logfilenames.sort(reverse = options.reverse)\n" \
"\n" \
"    if options.debug:\n" \
"        print(logfilenames)\n" \
"\n" \
"    if options.merge or (not logdict or YTAG):\n" \
"        if options.merge: # First priority checking\n" \
"            merge_logs(logfilenames, os.path.join(analyzer_relative_path, merged))\n" \
"        elif YTAG: # Second priority checking\n" \
"            ytag_folder_abspath = os.path.join(analyzer_relative_path, ytag_folder)\n" \
"            if os.access(ytag_folder_abspath, os.F_OK):\n" \
"                shutil.rmtree(ytag_folder_abspath)\n" \
"            os.mkdir(ytag_folder_abspath)\n" \
"            tmp_file = os.path.join(ytag_folder_abspath, 'temp.log')\n" \
"            merge_logs(logfilenames, tmp_file)\n" \
"            ytag_parse(tmp_file)\n" \
"            os.remove(tmp_file)\n" \
"        else: # Last priority checking\n" \
"            merge_logs(logfilenames, os.path.join(analyzer_relative_path, merged))\n" \
"    else:\n" \
"        split_log(logfilenames, logdict)\n" \
"\n" \
"    if options.remove:\n" \
"        for log in logfilenames:\n" \
"            os.remove(os.path.join(analyzer_relative_path, log))\n" \
"        os.remove(sys.argv[0])\n" \
"\n" \
"if __name__ == '__main__':\n" \
"    main()\n" \
""
