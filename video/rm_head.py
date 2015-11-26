import os


os.system('ls ./include_ac702 > list')

with open('list') as fs:
  for e_file in fs:
    cmd = 'find ./ -path ./include_ac702 -prune -o -type f -name ' + e_file.strip() + ' -exec git rm {} \;'
    #print(cmd)
    os.system(cmd)

os.system('rm ./list')

