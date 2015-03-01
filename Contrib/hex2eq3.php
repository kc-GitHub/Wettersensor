#! /usr/bin/php -q
<?php

define('LF', "\n");

class converter {

	protected $magicWord = '0x11 0x47';											// magic word string for tag updatefile as bootloader
	protected $tmpFile = 'tmpfile.tmp';
	protected $options = array(
		'help',
		'inFile:',
		'outFile:',
		'outFormat:',
		'spmPageSize:',
		'pathTo-srec_cat:',
		'hexEndAddress:',
		'withCrcCheck',
		'markAsBootloaderUpdate',
	);

	protected $thisScript = '';
	protected $thisScriptPathAbs = '';
	protected $inFile;
	protected $outFile;
	protected $outFormat;
	protected $spmPageSize;
	protected $pathToSrecCat;
	protected $hexEndAddress;
	protected $withCrcCheck;
	protected $markAsBootloaderUpdate;

	/**
	 * The constructor
	 */
	public function __construct($argv) {
		$this->thisScript = $argv[0];
		$this->init();
	}

	protected function init() {
		$pathParts = pathinfo(__file__);
		$this->thisScriptPathAbs = $pathParts['dirname'];

		$options = getopt('h', $this->options);

		if (array_key_exists('h', $options) || array_key_exists('help', $options) ||
			!array_key_exists('inFile', $options) || (array_key_exists('inFile', $options) && !$options['inFile']) ) {

			$this->printHelpAndExit();
		} else {
			$this->inFile                 = $options['inFile'];
			$this->outFormat              = (array_key_exists('outFormat' , $options)       && $options['outFormat'])        ? $options['outFormat']       : 'eq3';
			$this->spmPageSize            = (array_key_exists('spmPageSize' , $options)     && $options['spmPageSize'])      ? $options['spmPageSize']     : 128;
			$this->pathToSrecCat          = (array_key_exists('pathTo-srec_cat' , $options) && $options['pathTo-srec_cat'])  ? $options['pathTo-srec_cat'] : $this->thisScriptPathAbs . '/bin/srecord/srec_cat';
			$this->hexEndAddress          = (array_key_exists('hexEndAddress' , $options)   && $options['hexEndAddress'])    ? $options['hexEndAddress']   : '0x6FFE';
			$this->hexEndAddress          = ((int)$this->hexEndAddress == $this->hexEndAddress)                              ? (int)$this->hexEndAddress   : hexdec($this->hexEndAddress);

			$this->withCrcCheck           = (array_key_exists('withCrcCheck' , $options))                                    ? true                        : false;
			$this->markAsBootloaderUpdate = (array_key_exists('markAsBootloaderUpdate' , $options))                          ? true                        : false;
		}

		if ($this->spmPageSize != 64 && $this->spmPageSize != 128 && $this->spmPageSize != 256 && $this->spmPageSize != 512) {
			$this->printHelpAndExit();
		}

		if ($this->outFormat != 'eq3' && $this->outFormat != 'hex' && $this->outFormat != 'bin') {
			$this->printHelpAndExit();
		}

		// check access for inFile
		$fp = @fopen($this->inFile, 'r');
		if (!$fp) {
			print ('Could not open ' . $this->inFile . LF);
			exit();
		}
		fclose($fp);

		$pathParts = pathinfo($this->inFile);
		$defautlOutFileName = './' . $pathParts['filename'] . '.' . $this->outFormat;

		$this->outFile       = (array_key_exists('outFile' , $options)         && $options['outFile'])          ? $options['outFile']         : $defautlOutFileName;

		if ($this->inFile == $this->outFile) {
			print ('inFile and outFile can not be the same' . LF);
			exit();
		}
	}

	protected function printHelpAndExit() {
		print ('Commandline:' . LF);
		print ($this->thisScript . ' --inFile <infile.hex> --outFile <outfile> --spmPageSize <64|128|256|512> [--hexEndAddress <hexEndAddress>] [--outFormat <eq3|hex|bin>] [--markAsBootloaderUpdate] [--withCrcCheck --pathTo-srec_cat <pathTo-srec_cat>]' . LF);
		print (LF);

		exit();
	}

	protected function bin2eq3() {
		// check access for outFile
		$fp = @fopen($this->outFile, 'w');
		if (!$fp) {
			print ('Could not write to ' . $this->outFile . LF);
			exit();
		} else {
			fclose ($fp);
		}

		// check access for inFile
		$fp = @fopen($this->tmpFile, 'r');
		if (!$fp) {
			print ('Could not open ' . $this->tmpFile . LF);
			exit();
		}

		$out = '';
		while(!feof($fp)) {
			$payload = fread($fp, $this->spmPageSize);
			if (strlen($payload) > 0) {
				$out.= sprintf('%04X', $this->spmPageSize);

				for($i = 0; $i < $this->spmPageSize; $i++) {
					if ($i >= strlen($payload)) {
						$out.= '00';
					} else {
						$out.= sprintf('%02X', ord($payload[$i]));
					}
				}
			}
		}

		file_put_contents($this->outFile, $out);
	}

	public function convert() {
		if (!file_exists($this->pathToSrecCat)) {
			print ('Could not found srec_cat at: ' . $this->pathToSrecCat . LF);
			exit();
		}

		switch ($this->outFormat) {
			case 'eq3':
			case 'bin':
				$outFormat = 'binary';
				break;

			case 'hex':
				$outFormat = 'intel';
				break;
		}

		$hexEndAddress = sprintf('0x%04x', $this->hexEndAddress);
		$crcCmd = ($this->withCrcCheck) ? ' -Cyclic_Redundancy_Check_16_Little_Endian ' . $hexEndAddress : '';

		if ($this->markAsBootloaderUpdate) {
			$magigWordAdress = sprintf('0x%04x', $this->hexEndAddress + 2);
			$blEndAddress    = sprintf('0x%04x', $this->hexEndAddress - 2);
			$srecCatCmdBootloaderFlag = $this->pathToSrecCat . ' ' . $this->inFile .
				' -intel -offset -' . $magigWordAdress . ' -fill 0xff 0x0000 ' .$blEndAddress .
				' -generate ' .$blEndAddress . ' ' . $hexEndAddress .
				' -repeat-data ' . $this->magicWord . ' -o ' . $this->tmpFile . '.bin -binary';

			exec($srecCatCmdBootloaderFlag);

			$srecCatCmd = $this->pathToSrecCat . ' ' . $this->tmpFile . '.bin -binary' . $crcCmd . ' -o ' . $this->tmpFile . ' -' . $outFormat;
		} else {
			$srecCatCmd = $this->pathToSrecCat . ' ' . $this->inFile . ' -intel -fill 0xFF 0x0000 ' . $hexEndAddress . $crcCmd . ' -o ' . $this->tmpFile . ' -' . $outFormat;
		}

		exec($srecCatCmd);

		if ($this->outFormat == 'eq3') {
			$this->bin2eq3();
			@unlink($this->tmpFile.'.bin');
			unlink($this->tmpFile);
		} else {
			rename($this->tmpFile, $this->outFile);
		}
	}
}

$converter = new converter($argv);
$converter->convert();

exit();
