Phase90Ch : MultiOutUGen {
	*ar { arg in = 0.0, rate = 1.0, depth = 1.0, feedback = 0.6, mix = 0.5, fbStage = 2, stereo = 0, mul = 1.0, add = 0.0;
		var channels = if(stereo > 0.0, 2, 1);
		^this.multiNew('audio', channels, in, rate, depth, feedback, mix, fbStage, stereo).madd(mul, add)
	}

	init { arg numChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(numChannels, rate);
	}

	argNamesInputsOffset { ^2 }
}
