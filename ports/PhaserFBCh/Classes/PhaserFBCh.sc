PhaserFBCh : UGen {
	*ar { arg in = 0.0, lfo = 0.0, skew = 0.0, feedback = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, lfo, skew, feedback).madd(mul, add)
	}
}
