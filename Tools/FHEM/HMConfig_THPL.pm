package main;

use strict;
use warnings;

# device definition
$HMConfig::culHmModel{'F101'}             = {name => 'HB-UW-Sen-THPL-I',   st   => 'THPLSensor',   cyc  => '00:10',   rxt  => 'c:f',   lst  => 'p',   chn  => '',};
$HMConfig::culHmModel{'F102'}             = $HMConfig::culHmModel{'F101'};

# Register model mapping
$HMConfig::culHmRegModel{'HB-UW-Sen-THPL-I'}      = {burstRx =>1};
$HMConfig::culHmRegModel{'HB-UW-Sen-THPL-O'}      = $HMConfig::culHmRegModel{'HB-UW-Sen-THPL-I'};

# subtype channel mapping
$HMConfig::culHmSubTypeSets{'THPLSensor'} = {peerChan => '0 <actChn> ... single [set|unset] [actor|remote|both]'};

# Subtype spezific funtions
sub CUL_HM_ParseTHPLSensor(@){
	
	my ($mFlg, $frameType, $src, $dst, $msgData, $targetDevIO) = @_;
	
	my $shash = CUL_HM_id2Hash($src);                                           #sourcehash - will be modified to channel entity
	my @events = ();

	# WEATHER_EVENT
	if ($frameType eq '70'){
		my $name = $shash->{NAME};
		my $chn = '01';

		my ($dTempBat, $hum, $pressure, $lux, $batVoltage) = map{hex($_)} unpack ('A4A2A4A8A4A8A8', $msgData);

		my $temperature =  $dTempBat & 0x7fff;
		$temperature = ($temperature &0x4000) ? $temperature - 0x8000 : $temperature; 
		$temperature = sprintf('%0.1f', $temperature / 10);

		my $stateMsg = 'state:T: ' . $temperature;
		push (@events, [$shash, 1, 'temperature:' . $temperature]);
		push (@events, [$shash, 1, 'battery:' . ($dTempBat & 0x8000 ? 'low' : 'ok')]);

		$lux = ($lux + 0.0) / 100;
		$lux = ($lux < 100) ? $lux : sprintf('%.0f', $lux);
		$batVoltage = sprintf('%.2f', (($batVoltage + 0.00) / 1000));

		if ($modules{CUL_HM}{defptr}{$src.$chn}){
			my $ch = $modules{CUL_HM}{defptr}{$src.$chn};
			push (@events, [$ch, 1, $stateMsg]);
			push (@events, [$ch, 1, 'temperature:' . $temperature]);
		}

		# humidity
		if ($hum)                 {
			$stateMsg .= ' H: ' . $hum;
			push (@events, [$shash, 1, 'humidity:'   . $hum]);
		}
		
		# luminosity
		if ($lux < 65538) {
			$stateMsg .= ' Lux: ' . $lux;
			push (@events, [$shash, 1, 'lux:'        . $lux]);
		}

		# air pressure
		if ($pressure) {
			$stateMsg .= ' P: '    . $pressure;
			push (@events, [$shash, 1, 'pressure:'    . $pressure]);

			my $altitude = AttrVal('global', 'altitude', 0);
			my $pressureNN = $altitude ? sprintf('%.0f', ($pressure + ($altitude / 8.5))) : 0;
			if ($pressureNN) {
				$stateMsg .= ' P-NN: ' . $pressureNN;
				push (@events, [$shash, 1, 'pressure-nn:' . $pressureNN]);
			}
		}

		$stateMsg .= ' batVoltage: ' . $batVoltage;  push (@events, [$shash, 1, 'batVoltage:' . $batVoltage]);
		
		push (@events, [$shash, 1, $stateMsg]);
	}

	return @events;
}

1;
