TapeChewCh : UGen {
	*ar { arg in = 0.0, depth = 0.0, frequency = 0.0, variance = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, depth, frequency, variance).madd(mul, add)
	}
}
