KickCh : UGen {
	*ar { arg trig = 0.0, freq = 100.0, pulseWidthMs = 1.0, pulseAmp = 1.0, velocity = 1.0, voices = 1.0,
		sustain = 0.5, decay = 0.5, q = 0.5, damping = 0.5, tight = 0.5, bounce = 0.0, mode = 1.0,
		portamentoMs = 50.0, noiseAmt = 0.0, noiseDecay = 0.5, noiseCutoff = 2000.0, noiseType = 0.0,
		tone = 800.0, levelDB = 0.0, mul = 1.0, add = 0.0;
		^this.multiNew(
			'audio',
			trig, freq, pulseWidthMs, pulseAmp, velocity, voices,
			sustain, decay, q, damping, tight, bounce, mode,
			portamentoMs, noiseAmt, noiseDecay, noiseCutoff, noiseType,
			tone, levelDB
		).madd(mul, add)
	}
}
