#/bin/bash
if [ "$1" = "y" ];then
echo "Update the binary header file of HTML file"
current_path=$(readlink -f "$(dirname "$0")")
gzip -cf ${current_path}/setting_ssid.html > ${current_path}/setting_ssid.html.gz
${current_path}/filetoarray ${current_path}/setting_ssid.html.gz > ${current_path}/setting_ssid.h
echo "Binary header file has been generated"
fi 

