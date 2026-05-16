ARPFilterCh : UGen {
	*ar { arg in = 0.0, freq = 1000.0, q = 0.70710678, limitMode = 0.0, notchOffset = 0.0, mode = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq, q, limitMode, notchOffset, mode).madd(mul, add)
	}
}
