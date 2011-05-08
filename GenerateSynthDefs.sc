var synthDefPath = "~/Dropbox/Soundfood/synthdefs".standardizePath;
SynthDef.synthDefDir = synthDefPath;
~synthPath = "~/Dropbox/Soundfood/Synths/*".standardizePath;
~effectPath = "~/Dropbox/Soundfood/Effects/*".standardizePath;

~soundWormSynthCreator = {|name, synthUGen|
    // Creates appropriately enveloped synths for percussion sounds
    SynthDef(name, {|inBus=0, outBus=0, gate=1, pitch=0|
	    var input = In.ar(inBus, 1);
        var volumeEnvelope = EnvGen.ar(
            envelope:Env.perc,
            gate:gate,
            doneAction:2
        );
        var synth = Pan2.ar(
            synthUGen.(pitch, input),
            pos:Rand(-1, 1)
        );
        Out.ar(outBus, volumeEnvelope * synth)
    }).load(s);
};

fork {
    s.bootSync;
    
    //~loadTestSynth.();
    //~loadStressSynth.(200);
    
    ~synthPath.pathMatch.do{|filePath|
	    var synthName = filePath.basename.splitext[0];
	    "loading ".post; filePath.postln;
        ~soundWormSynthCreator.(synthName, filePath.load);
    };
    // Exit SuperCollider at end of compile script
    0.exit;
}
