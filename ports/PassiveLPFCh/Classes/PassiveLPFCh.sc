PassiveLPFCh : UGen {
	*ar { arg in = 0.0, freq = 1000.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, freq).madd(mul, add)
	}
}
