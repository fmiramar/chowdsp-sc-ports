LCOscCh : UGen {
	*ar { arg in = 0.0, freq = 200.0, closed = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq, closed).madd(mul, add)
	}
}
