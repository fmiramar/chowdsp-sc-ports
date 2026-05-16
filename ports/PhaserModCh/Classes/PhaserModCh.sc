PhaserModCh : UGen {
	*ar { arg in = 0.0, lfo = 0.0, skew = 0.0, mod = 0.0, stages = 8.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, lfo, skew, mod, stages).madd(mul, add)
	}
}
