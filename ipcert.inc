<?PHP

class ip_cert {
	const NOT_FOUND=0;
	const CERTIFIED=1;
	const REFERENCE=2;

	private $initial_uris=[ 'https://soap-001.mazana.tv/ipcert/?function=get_certified_ip&service_id={service_id}&use_key={use_key}',
		'https://soap-001.mazana.tv/ipcert/?function=get_certified_ip&service_id={service_id}&use_key={use_key}&try=2',
		'https://soap-001.mazana.tv/ipcert/?function=get_certified_ip&service_id={service_id}&use_key={use_key}&try=3',
		'https://soap-001.mazana.tv/ipcert/?function=get_certified_ip&service_id={service_id}&use_key={use_key}&try=4',
	];

	private $failsafe_uris=[ 'https://soap-001.mazana.tv/ipcert/?xxxx=failsafe&function=get_certified_ip&service_id={service_id}&use_key={use_key}',
		'https://soap-001.mazana.tv/ipcert/?xxxx=failsafe&function=get_certified_ip&service_id={service_id}&use_key={use_key}&try=2',
		'https://soap-001.mazana.tv/ipcert/?xxxx=failsafe&function=get_certified_ip&service_id={service_id}&use_key={use_key}&try=3',
		'https://soap-001.mazana.tv/ipcert/?xxxx=failsafe&function=get_certified_ip&service_id={service_id}&use_key={use_key}&try=4',
	];
	
	

	private $service_id, $use_key; // 1eAF3C3LyPmm4HA
	private $http_code=NULL, $report_msg=NULL;

	public $state, $ip, $ip_type, $cookie, $remote_debug=NULL, $backtrace_debug=[];
	
	public function get_certified_ip() {
		return($this->ip);
	}

	public function get_certified_ip_type() {
		return($this->ip_type);
	}
	
	public function get_cookie() {
		return($this->cookie);
	}

	public function get_state() {
		return($this->state);
	}

	public function get_state_text() {
		switch ($this->state) {
			case self::NOT_FOUND: return('IP not found or check error');
			case self::CERTIFIED: return('IP is certified');
			case self::REFERENCE: return('IP is uncertified, for reference only');
			default: return('internal error');
		}
	}

	public function get_debug() {
		return($this->backtrace_debug);
	}

	public function __construct($service_id, $use_key, $initial_uris=NULL, $failsafe_uris=NULL) {	
		if (!$service_id || !$use_key) throw new Exception('service_id and use_key must not be empty');
		$this->service_id=$service_id;
		$this->use_key=$use_key;
		if ($initial_uri) $this->initial_uri=$initial_uri;
		if ($failsafe_uris) $this->failsafe_uris=$failsafe_uris;
		$this->do_fail();
	}
	
	private function hydrate_uri($initial_uri, $service_id, $use_key) {
		$uri=str_replace('{service_id}', $service_id, $initial_uri);
		$uri=str_replace('{use_key}', $use_key, $uri);
		return($uri);
	}
	
	
	private function add_uris(&$queue, $primary, $backup) {
		if ($backup) {
			shuffle($backup);
			$queue=array_merge($backup, $queue); // We add randomized order of backup servers to the top
		}
		if ($primary) {
			shuffle($primary);
			array_unshift($queue, array_shift($primary)); // and one random primary to top
		}
	}
		
	
	private function select_uri(&$try_uris, &$back_uris, &$type) {
		$type=='primary' ? $set='try_uris' : $set='back_uris';
		$c=count($$set);
		if (!$c) {
			if ($type=='failsafe') return(NULL);
			$type='failsafe';
			$set='back_uris';
			$c=count($$set);
			if (!$c) return(NULL);
		}
		$i=rand(0, $c-1);
		$ret=$$set[$i];
		unset($$set[$i]);
		$$set=array_values($$set);
		return($ret);
	}
	
	private function do_fail($report='') {
		$this->state=self::NOT_FOUND;
		$this->ip=NULL;
		$this->ip_type=NULL;
		$this->cookie=NULL;
		if ($report) $this->backtrace_debug[]=$report;
	}
	
	public function certify_ip() {
		$try=10;
		$try_uris=[];
		$this->add_uris($try_uris, $this->initial_uris, $this->failsafe_uris);
		while($try) {
			$try--;
			$uri=array_shift($try_uris);
			if (!$uri) break;
			$this->backtrace_debug[]="try $uri";
			$uri=$this->hydrate_uri($uri, $this->service_id, $this->use_key);
			$this->backtrace_debug[]="hydrated to $uri";
			if (!$response=$this->perform_get($uri)) {
				$this->backtrace_debug[]="HTTP failed: $this->report_msg";
				continue;
			}
			if (!$response) {
				$this->backtrace_debug[]="returned empty data";
				continue;
			}
			$response=json_decode($response, TRUE);
			if (!$response || !isset($response['status'])) {
				$this->backtrace_debug[]="returned unparasble JSON or incomplete response";
				continue;
			}
			if ($response['status']=='redirect') {
				$this->add_uris($try_uris, $response['primary'], $response['failsafe']) ;
				// Wee will prepend redirection first, when it fails return to original backups, but just first of primary is used
				continue;
			}

			$this->ip=$response['ip'];
			$this->ip_type=$response['ip_type'];
			$this->cookie=$response['cookie'];
			if (isset($response['debug'])) $this->backtrace_debug[]="remote debug: $response[debug]";
			
			switch ($response['status']) {
				case 'reference':
					$this->state=self::REFERENCE;
					return(TRUE);
				case 'certified':
					$this->state=self::CERTIFIED;
					return(TRUE);
				default:
					$this->backtrace_debug[]="ret unknown status $response[status]";
			}
		}
		if ($try) $this->backtrace_debug[]="all possible URIs failed";
		else $this->backtrace_debug[]="too many redirections";
	}

 
	private function perform_get($endpoint, $timeout="3") {
		$ch = curl_init();
		curl_setopt($ch, CURLOPT_URL, $endpoint);
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
		curl_setopt($ch, CURLOPT_POST, false);
		curl_setopt($ch, CURLOPT_HTTPGET, 1);
		curl_setopt($ch, CURLOPT_TIMEOUT, $timeout);
		$ret=curl_exec($ch);
		$this->http_code=curl_getinfo($ch, CURLINFO_HTTP_CODE);
		curl_close($ch);
		if ($this->http_code<200 || $this->http_code>=300) {
			$ret=trim($ret);
			$this->report_msg="HTTP $this->http_code: $ret";
			return(FALSE);
		}
//print_r($ret);
		return($ret);
	}
}
 