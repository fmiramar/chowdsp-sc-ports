FracFilterCh : UGen {
	*ar { arg in = 0.0, freq = 1000.0, alpha = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq, alpha).madd(mul, add)
	}
}
