FDNCh : UGen {
	*ar { arg in = 0.0, preDelay = 0.5, size = 0.5, t60High = 0.5, t60Low = 1.0, numDelays = 4.0, mix = 1.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, preDelay, size, t60High, t60Low, numDelays, mix).madd(mul, add)
	}
}
