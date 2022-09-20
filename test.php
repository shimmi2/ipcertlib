<?PHP
require('ipcert.inc');

// one of configured service on head end side
$service_id='mazana.tv';

// use key for this client type
$use_key='1eAF3C3LyPmm4HA';

//__construct($service_id, $use_key, $initial_uris=NULL)
$ipcert=new ip_cert($service_id, $use_key);

echo "certifying our IP for $service_id...\n";
$ipcert->certify_ip();


echo "done\n";
echo "\nnumber status: ".$ipcert->get_state();
echo "\ntext status: ".$ipcert->get_state_text();
echo "\nIP: ".$ipcert->get_certified_ip();
echo "\nIP type: ".$ipcert->get_certified_ip_type();
echo "\ncookie: ".$ipcert->get_cookie();
echo "\ntracing debug below:\n";
print_r($ipcert->get_debug());

?>