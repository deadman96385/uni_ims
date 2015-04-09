import os

src_dir = './'
dst_dir = '../vPort_R8/'
cmd = 'find ' + dst_dir + ' -type f -name config.mk -execdir mv {} config_D2.mk \;'
print(cmd)
os.system(cmd)

os.system('find ' + src_dir + ' -type f -name Android.mk > tmp.txt')

with open('tmp.txt') as ftmp:
    for e_line in ftmp:
        src_file = e_line.strip()
        [na, nb] = e_line.strip().split(src_dir)
        dst_file = dst_dir + nb
        #copy Andoird.mk
        cmd = 'cp ' + src_file + ' ' + dst_file
        print(cmd)
        os.system(cmd)

        # cp config.mk
        [nc, nd] = os.path.split(nb.strip())
        src_file = src_dir + nc + '/config.mk'
        if os.path.exists(src_file):
            dst_file = dst_dir + nc + '/config.mk'
            cmd = 'cp ' + src_file + ' ' + dst_file
            print(cmd)
            os.system(cmd)

os.system('rm tmp.txt')
os.system('find -name Makefile -exec rm {} \;')

file_mod_list = ['vPort_R8/osal/user/bionic/osal_sys.c',
	'modemDriver/vpad/main/vpad_main.c',
	'system-ac702.mk',
	'system-malibu.mk',
	'README.txt',
	'osal/kernel/arch/ac702/osal_platform.h',
	'include_ac702/osal_platform.h']

for e_elem in file_mod_list:
	src_file = src_dir + e_elem
	dst_file = dst_dir + e_elem
	if os.path.exists(src_file):
		cmd = 'cp ' + src_file + ' ' + dst_file
		print(cmd)
		os.system(cmd)

