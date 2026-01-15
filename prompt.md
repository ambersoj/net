
alias startxfr='
../bls/bls 4000 & \
#
../net/net 5000 & \
../tck/tck-fsm-net 5001 & \
../tck/tck-net 5003 & \
./fsm-net 5002 & \
#
../xfr/xfr 6000 & \
../tck/tck-fsm-xfr 6001 & \
./fsm-xfr 6002 &
'

     - printf '{"verb":"PUT","resource":"fsm","body":{"fsm_text":%s,"target_sba":5000, "tck_sba":5001}}' "$(jq -Rs . < fsm-net.puml)" | nc -u -w1 127.0.0.1 5002

     - printf '{"verb":"PUT","resource":"fsm","body":{"fsm_text":%s,"target_sba":6000, "tck_sba":6001}}' "$(jq -Rs . < fsm-xfr.puml)" | nc -u -w1 127.0.0.1 6002

     - echo '{"enable": true, "target_sba":5000}' | nc -u -w1 127.0.0.1 5003
     - echo '{"enable": true, "target_sba":5002}' | nc -u -w1 127.0.0.1 5001
     - echo '{"enable": true, "target_sba":6002}' | nc -u -w1 127.0.0.1 6001

echo '{"belief":{"subject":"FSM.XFR.start","polarity":true}}'  | nc -u -w1 127.0.0.1 4000

