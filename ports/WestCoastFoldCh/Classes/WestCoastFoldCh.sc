WestCoastFoldCh : UGen {
	*ar { arg in = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in).madd(mul, add)
	}
}
