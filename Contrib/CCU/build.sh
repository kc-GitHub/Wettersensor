#!/bin/sh

cd HB-UW-Sen-THPL_CCU-addon-src
chmod +x update_script
tar -H gnu -zcvf HB-UW-Sen-THPL_CCU-addon.tgz *
mv HB-UW-Sen-THPL_CCU-addon.tgz ..
cd ..