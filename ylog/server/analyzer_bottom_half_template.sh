#!/bin/bash
file_name_base="analyzer_bottom_half_template"
cat > ${file_name_base}.h <<__EOF
/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

/**
 * Auto created by ${file_name_base}.sh
 * base on ${file_name_base}.py
 * $(date -R)
 */

#define ${file_name_base} \\
__EOF
sed 's/\\/\\\\/g;s/"/\\"/g;s/\(.*\)/"\1\\n" \\/' ${file_name_base}.py >> ${file_name_base}.h
echo '""' >> ${file_name_base}.h
