VogueLadderCh : MultiOutUGen {
	*ar { arg in = 0.0, hpCutoff = 0.0, hpResonance = 0.0, lpCutoff = 1.0, lpResonance = 0.0, drive = 0.5, oscillating = 0.0, stereo = 0, mul = 1.0, add = 0.0;
		var channels = if(stereo > 0.0, 2, 1);
		^this.multiNew('audio', channels, in, hpCutoff, hpResonance, lpCutoff, lpResonance, drive, oscillating).madd(mul, add)
	}

	init { arg numChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(numChannels, rate);
	}

	argNamesInputsOffset { ^2 }
}
