JuniorBCh : UGen {
	*ar { arg in = 0.0, drive = 0.5, blend = 1.0, stages = 2, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, drive, blend, stages).madd(mul, add)
	}
}
