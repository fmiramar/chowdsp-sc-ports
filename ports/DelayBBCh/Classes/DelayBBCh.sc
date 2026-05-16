DelayBBCh : UGen {
	*ar { arg in = 0.0, delayTime = 0.08, filterFreq = 10000.0, feedback = 0.2, driveDB = 0.0, mix = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, delayTime, filterFreq, feedback, driveDB, mix).madd(mul, add)
	}
}
