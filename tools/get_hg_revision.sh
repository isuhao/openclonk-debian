#!/bin/sh
cd `dirname $0`
echo "#ifndef OC_HG_REVISION"
echo "#define OC_HG_REVISION \"`hg id --id`\""
echo "#endif"

