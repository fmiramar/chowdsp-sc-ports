TR808OutCh : UGen {
	*ar { arg in = 0.0, volume = 0.8, tone = 0.5, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, volume, tone).madd(mul, add)
	}
}
