#!/usr/bin/perl

use Cwd 'abs_path';
use File::Basename;

##############
# Flash-Tool #
##############

sub explainUsageAndExit(@);
sub checkHex ($);

###############################################################################

if(int(@ARGV) < 2) {
	explainUsageAndExit();
}

my $serialDevice = $ARGV[0];
my $hexFile      = $ARGV[1];
my $hmID         = $ARGV[2];
my $serialNr     = $ARGV[3];

if ( !(-e "$hexFile") ) {
    explainUsageAndExit("Can't access the given hexfile.");
}

##########################
# Set the default values #
##########################
my $defHmId1        = 'AB';
my $defHmId2        = 'CD';
my $defHmId3        = 'EF';
my $hmId1           = '';
my $hmId2           = '';
my $hmId3           = '';
my $defSerialNumber = 'HB0Default';
my $curPath         = $dirname = dirname(abs_path($0));
my ($fileBasename, $filePath, $fileExt) = fileparse($hexFile, '\..*');

##########################
# convert hexfile to bin #
##########################
`$curPath/bin/hex2bin -c $hexFile`;

if (defined($hmID) && defined($serialNr)) {
	#########################
	# Check the valid HM-ID #
	#########################
	($hmId1, $hmId2, $hmId3) = split(/:/, $hmID);

	if (!checkHex($hmId1) || !checkHex($hmId2) || !checkHex($hmId3)) {
		explainUsageAndExit ("The entered HM-ID $hmID is invalid. Format: XX:XX:XX. Each X must be 0-9 or A-F.");
	} else {
		$hmID = $hmId1 . $hmId2 . $hmId3;
	}

	#################################
	# Check the valid serial number #
	#################################
	if ( !($serialNr =~ /^[0-9a-zA-Z]{10}$/) ) {;
		explainUsageAndExit ("The serial number must contains 10 characters 0-9 or A-Z.");
	}

	##########################################################
	# Write user defined HM-ID and serial number to bin-file #
	##########################################################
	my $sedParams = '"s/\(' . $defSerialNumber . '\)\(.*\)\(\x' . $defHmId1 . '\x' . $defHmId2 . '\x' . $defHmId3 . '\)/' . $serialNr . '\2\x' . $hmId1 . '\x' . $hmId2 . '\x' . $hmId3.'/" ' . $curPath . '/' . $fileBasename . '.bin > ' . $curPath . '/' . $fileBasename . '.tmp';
	`sed -b -e $sedParams`;

	unlink "$curPath/$fileBasename.bin";
	rename "$curPath/$fileBasename.tmp", "$fileBasename.bin";
} else {
	$hmId1    = $defHmId1;
	$hmId2    = $defHmId2;
	$hmId3    = $defHmId3;
	$serialNr = $defSerialNumber;
}

######################################################
# Reset sensor board via gpio pin 17 on Raspberry Pi #
######################################################
`if test ! -d /sys/class/gpio/gpio18; then echo 18 > /sys/class/gpio/export; fi`;
`echo out > /sys/class/gpio/gpio18/direction`;
`echo 0 > /sys/class/gpio/gpio18/value`;
select(undef, undef, undef, 0.5); 
`echo 1 > /sys/class/gpio/gpio18/value`;

###############################
# Begin flashing with avrdude #
###############################
print "Start flashing device with HM-ID: $hmId1:$hmId2:$hmId3 and serial number: $serialNr \n";

`$curPath/bin/avrdude -C$curPath/bin/avrdude.conf -patmega328p -carduino -P $serialDevice -b57600 -D -Uflash:w:$fileBasename.bin:r`;
unlink "$fileBasename.bin";

print "$curPath/avrdude -C$curPath/avrdude.conf -patmega328p -carduino -P $serialDevice -b57600 -D -Uflash:w:$fileBasename.bin:r \n";
exit (0);


##########################
# Write out param errors #
##########################
sub explainUsageAndExit(@) {
	my ($txt) = @_;

	print "\n";
	if (defined($txt)) {
		print "$txt\n"
	}

	print "Usage: flash.sh <SerialDevice> <hexfile> [<HM-ID> <Serial>]\n";
	exit (0);
}


#######################################
# Check a variable if is a hex number #
#######################################
sub checkHex ($) {
	my ($val) = @_;
	my $retVal = 0;
	
	if ($val =~ /^[0-9a-fA-F]{2}$/) {;
		$retVal = 1;
	}

	return $retVal;
}
