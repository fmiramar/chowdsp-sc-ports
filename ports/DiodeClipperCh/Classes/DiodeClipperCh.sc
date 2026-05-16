DiodeClipperCh : UGen {
	*ar { arg in = 0.0, cutoff = 1000.0, gainDB = 0.0, outDB = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, cutoff, gainDB, outDB).madd(mul, add)
	}
}
