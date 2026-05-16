TapeDegradeCh : UGen {
	*ar { arg in = 0.0, depth = 0.0, amount = 0.0, variance = 0.0, envelope = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew('audio', in, depth, amount, variance, envelope).madd(mul, add)
	}
}
