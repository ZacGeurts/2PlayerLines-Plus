// songgen.h - picks the random song using defined musical rules.

// This is not free software and requires royalties for commercial use.
// Royalties are required for songgen.cpp songgen.h instruments.h and instrument files
// Interested parties can find my contact information at https://github.com/ZacGeurts
// If you make commercial gain, you can do the math and update my Patreon.

// Follow the local law.
// FCC in the USA restricts what frequencies you can broadcast.
// Most, if not all countries restrict frequencies.
// Audible frequencies in the USA are a First Amendment Right. 
// There are some nefarious limitations, like faking a policeman phone call, but making sound is a human right.
// Maybe there is a usage for frequencies exceeding this many 31415926535897932384626433832795029L
/* 	
	Far out of this scope. I cap to 20hz and less than 44100hz (SDL2 maximum and exceeds human hearing).
 	We can go below 20hz down to 0hz, but top of the line car stereos might hit 8hz-12hz with expensive equipment.
 	It requires too much voltage to go lower and you would not hear a difference.
 	20hz-80hz should be top quality for a subwoofer and it does not try blowing out pc speakers.
*/
// You would need more than a speaker. Fun fact: WiFi is frequencies. Sound you cannot hear.
// Do not restrict emergency communications or damage heart pace makers, etc.
// Always put hearing safety first. It does not grow back.
// Be kind to pets.

#ifndef SONGGEN_H
#define SONGGEN_H

const int MAX_INSTRUMENTS = 8; // do not exceed your instrument files.
// number of instruments permitted per song.
// 31 Instruments and 30 Genres at time of writing.

// The recommended method to add a song genre is to search this file for every instance of indie and Indie and INDIE, and modify it.
// Leave the genre name as indie to avoid issues if you miss changing a setting.

// initalize rng in your function before using it with 
// 'static thread_local AudioUtils::RandomGenerator rng;'
// static thread_local is not required if you want a global rng.
// I have yet to performance the current rng so for now I thread the generation.

#include "instruments.h"
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <set>
#include <tuple>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <ctime>
#include <numeric>
#include <filesystem>
#include <SDL2/SDL.h>
// #include <random> // nope

// adding a NEW Genre is currently no small task, I suggest modifying INDIE values.
namespace SongGen {
	enum Genre {
    	AMBIENT, BLUEGRASS, BLUES, CLASSICAL, CLASSICAL_JAZZ_FUSION, COUNTRY, DISCO, DUBSTEP, EDM, ELECTRONICA,
    	FOLK, FUNK, GOSPEL, HIPHOP, INDIE, JAZZ, LATIN, METAL, NEW_AGE, POP, PUNK, RAP, REGGAE, REGGAETON,
    	RNB, ROCK, SOUL, TECHNO, TRAP, WORLD
	};
	
	static const std::map<Genre, std::set<std::string>> getInstruments;
	
    struct Note {        	
		long double freq, duration, startTime;
		int phoneme; // used for vocal notes  
		bool open; // open and not closed hihat and maybe other uses
		long double volume, velocity;
		
		Note(long double freq = 440.0L, long double d = 0.0625L, long double s = 0.0L, int p = -1, bool o = false, long double v = 0.5L, long double vel = 0.8L)
            : freq(freq), duration(d), startTime(s), phoneme(p), open(o), volume(v), velocity(vel) {}
    };

	struct Part {
    	std::string instrument;
    	std::vector<Note> notes;		
    	bool useReverb;
    	long double reverbAirMix, reverbDelay, reverbDecay;
    	bool useDistortion;
    	long double pan, reverbMix;
    	std::string sectionName;
    	long double distortionDrive, distortionThreshold;
    	std::vector<std::pair<long double, long double>> panAutomation, volumeAutomation, reverbMixAutomation;
        
    	Part() : instrument(""), notes(), useReverb(false), reverbAirMix(0.2L), reverbDelay(0.1L),
            	 reverbDecay(0.5L), useDistortion(false), pan(0.0L), reverbMix(0.2L), sectionName(""),
        	     distortionDrive(1.5L), distortionThreshold(0.7L), panAutomation(), volumeAutomation(),
    	         reverbMixAutomation() {}
	};

	struct Section {
		std::string name, templateName;
		long double startTime, endTime, progress;
    
		Section(std::string n, std::string t, long double start, long double end, long double prog = 0.0L)
			: name(n), templateName(t), startTime(start), endTime(end), progress(prog) {}
	};

// Music Generator is the brains of songgen.
// ---
	class MusicGenerator {
    public:
	std::vector<SongGen::Part> parts;
	static std::map<std::string, Instruments::InstrumentFactory>& getInstrumentRegistry();
	
	std::vector<std::vector<std::tuple<std::string, std::string, long double>>> getSectionPlans(Genre g) {
    	using Section = std::tuple<std::string, std::string, long double>;
    	using Plan = std::vector<Section>;
    	using Plans = std::vector<Plan>;

    	// Common structure shared by multiple genres
    	static const Plans commonPop = {
        	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Chorus2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}},
        	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Bridge", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}}
    	};

    	// Map of genre-specific plans, with additional plans for genres that extend commonPop
    	static const std::map<Genre, Plans> genrePlans = {
        	{CLASSICAL, {
            	{{"Intro", "Intro", 0.0L}, {"Exposition", "Verse", 0.2L}, {"Development", "Chorus", 0.4L}, {"Recapitulation", "Verse", 0.6L}, {"Coda", "Outro", 0.8L}},
            	{{"Intro", "Intro", 0.0L}, {"PartA", "Verse", 0.25L}, {"PartB", "Chorus", 0.5L}, {"PartA2", "Verse", 0.75L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Section1", "Verse", 0.2L}, {"Section2", "Verse", 0.4L}, {"Section3", "Chorus", 0.6L}, {"Outro", "Outro", 0.8L}}
        	}},
        	{JAZZ, {
            	{{"Intro", "Intro", 0.0L}, {"Head1", "Verse", 0.2L}, {"Bridge", "Chorus", 0.4L}, {"Head2", "Verse", 0.6L}, {"Outro", "Outro", 0.8L}},
            	{{"Intro", "Intro", 0.0L}, {"Chorus1", "Chorus", 0.2L}, {"Solo", "Verse", 0.4L}, {"Chorus2", "Chorus", 0.6L}, {"Outro", "Outro", 0.8L}},
            	{{"Intro", "Intro", 0.0L}, {"Head1", "Verse", 0.2L}, {"Solo1", "Chorus", 0.4L}, {"Solo2", "Chorus", 0.6L}, {"Head2", "Verse", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{POP, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.15L}, {"PreChorus1", "PreChorus", 0.3L}, {"Chorus1", "Chorus", 0.45L}, {"Verse2", "Verse", 0.6L}, {"Chorus2", "Chorus", 0.75L}, {"Outro", "Outro", 0.9L}});
            	return plans;
        	}()},
        	{ROCK, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Solo", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}});
            	return plans;
        	}()},
        	{TECHNO, {
            	{{"Intro", "Intro", 0.0L}, {"Build1", "Verse", 0.2L}, {"Drop1", "Drop", 0.4L}, {"Break", "Verse", 0.6L}, {"Build2", "Verse", 0.8L}, {"Drop2", "Drop", 0.9L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Build", "PreChorus", 0.4L}, {"Drop1", "Drop", 0.6L}, {"Verse2", "Verse", 0.8L}, {"Drop2", "Drop", 0.9L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Section1", "Verse", 0.25L}, {"Break", "Chorus", 0.5L}, {"Section2", "Verse", 0.75L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{RAP, {
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Hook1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Hook2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Hook1", "Chorus", 0.35L}, {"Verse2", "Verse", 0.5L}, {"Bridge", "Bridge", 0.65L}, {"Hook2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{BLUES, {
            	{{"Intro", "Intro", 0.0L}, {"Chorus1", "Chorus", 0.2L}, {"Solo", "Verse", 0.4L}, {"Chorus2", "Chorus", 0.6L}, {"Outro", "Outro", 0.8L}},
            	{{"Intro", "Intro", 0.0L}, {"Head1", "Verse", 0.2L}, {"Solo1", "Chorus", 0.4L}, {"Solo2", "Chorus", 0.6L}, {"Head2", "Verse", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{COUNTRY, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.15L}, {"Chorus1", "Chorus", 0.3L}, {"Verse2", "Verse", 0.45L}, {"Bridge", "Bridge", 0.6L}, {"Chorus2", "Chorus", 0.75L}, {"Outro", "Outro", 0.9L}});
            	return plans;
        	}()},
        	{FOLK, [] {
           	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Verse3", "Verse", 0.8L}, {"Outro", "Outro", 1.0L}});
            	return plans;
        	}()},
        	{REGGAE, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Skank", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}});
            	return plans;
        	}()},
        	{METAL, {
            	{{"Intro", "Intro", 0.0L}, {"Riff1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Riff2", "Verse", 0.6L}, {"Breakdown", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Riff1", "Verse", 0.2L}, {"Riff2", "Chorus", 0.4L}, {"Solo", "Verse", 0.6L}, {"Riff3", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{PUNK, {
            	{{"Intro", "Intro", 0.0L}, {"Riff1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Riff2", "Verse", 0.6L}, {"Chorus2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Bridge", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{DISCO, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Groove", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}});
            	return plans;
        	}()},
        	{FUNK, {
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Groove", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Riff1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Riff2", "Verse", 0.6L}, {"Break", "Bridge", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{SOUL, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"CallResponse", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}});
            	return plans;
        	}()},
        	{GOSPEL, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"CallResponse", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}});
            	return plans;
        	}()},
        	{AMBIENT, {
            	{{"Intro", "Intro", 0.0L}, {"Section1", "Verse", 0.2L}, {"Section2", "Chorus", 0.4L}, {"Section3", "Verse", 0.6L}, {"Outro", "Outro", 0.8L}},
            	{{"Intro", "Intro", 0.0L}, {"PartA", "Verse", 0.25L}, {"PartB", "Chorus", 0.5L}, {"PartA2", "Verse", 0.75L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{EDM, {
            	{{"Intro", "Intro", 0.0L}, {"Build1", "Verse", 0.2L}, {"Drop1", "Drop", 0.4L}, {"Break", "Verse", 0.6L}, {"Build2", "Verse", 0.8L}, {"Drop2", "Drop", 0.9L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Build", "PreChorus", 0.4L}, {"Drop1", "Drop", 0.6L}, {"Verse2", "Verse", 0.8L}, {"Drop2", "Drop", 0.9L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{LATIN, {
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Montuno", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Section1", "Verse", 0.2L}, {"Section2", "Chorus", 0.4L}, {"Section3", "Verse", 0.6L}, {"Climax", "Bridge", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{HIPHOP, {
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Hook1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Hook2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Hook1", "Chorus", 0.35L}, {"Verse2", "Verse", 0.5L}, {"Bridge", "Bridge", 0.65L}, {"Hook2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{WORLD, {
            	{{"Intro", "Intro", 0.0L}, {"Section1", "Verse", 0.2L}, {"Section2", "Chorus", 0.4L}, {"Section3", "Verse", 0.6L}, {"Climax", "Bridge", 0.8L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Bridge", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{RNB, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"CallResponse", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}});
            	return plans;
        	}()},
        	{INDIE, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Solo", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}});
            	return plans;
        	}()},
        	{ELECTRONICA, {
            	{{"Intro", "Intro", 0.0L}, {"Build1", "Verse", 0.2L}, {"Drop1", "Drop", 0.4L}, {"Break", "Verse", 0.6L}, {"Build2", "Verse", 0.8L}, {"Drop2", "Drop", 0.9L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Section1", "Verse", 0.25L}, {"Section2", "Chorus", 0.5L}, {"Section3", "Verse", 0.75L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{DUBSTEP, {
            	{{"Intro", "Intro", 0.0L}, {"Build1", "Verse", 0.2L}, {"Drop1", "Drop", 0.4L}, {"Break", "Verse", 0.6L}, {"Build2", "Verse", 0.8L}, {"Drop2", "Drop", 0.9L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Build", "PreChorus", 0.4L}, {"Drop1", "Drop", 0.6L}, {"Verse2", "Verse", 0.8L}, {"Drop2", "Drop", 0.9L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{CLASSICAL_JAZZ_FUSION, {
            	{{"Intro", "Intro", 0.0L}, {"Head1", "Verse", 0.2L}, {"Solo1", "Chorus", 0.4L}, {"Head2", "Verse", 0.6L}, {"Solo2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Theme1", "Verse", 0.2L}, {"Theme2", "Chorus", 0.4L}, {"Development", "Bridge", 0.6L}, {"Theme3", "Verse", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{REGGAETON, {
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Montuno", "Bridge", 0.8L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Hook1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Hook2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{BLUEGRASS, [] {
            	Plans plans = commonPop;
            	plans.push_back({{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Chorus1", "Chorus", 0.4L}, {"Break", "Bridge", 0.6L}, {"Verse2", "Verse", 0.75L}, {"Chorus2", "Chorus", 0.9L}, {"Outro", "Outro", 1.0L}});
            	return plans;
        	}()},
        	{TRAP, {
            	{{"Intro", "Intro", 0.0L}, {"Verse1", "Verse", 0.2L}, {"Hook1", "Chorus", 0.4L}, {"Verse2", "Verse", 0.6L}, {"Hook2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"Build1", "Verse", 0.2L}, {"Drop1", "Chorus", 0.4L}, {"Verse1", "Verse", 0.6L}, {"Drop2", "Chorus", 0.8L}, {"Outro", "Outro", 1.0L}}
        	}},
        	{NEW_AGE, {
            	{{"Intro", "Intro", 0.0L}, {"Section1", "Verse", 0.2L}, {"Section2", "Chorus", 0.4L}, {"Section3", "Verse", 0.6L}, {"Climax", "Bridge", 0.8L}, {"Outro", "Outro", 1.0L}},
            	{{"Intro", "Intro", 0.0L}, {"PartA", "Verse", 0.25L}, {"PartB", "Chorus", 0.5L}, {"PartA2", "Verse", 0.75L}, {"Outro", "Outro", 1.0L}}
	        }}
    	};

    	// Return the plans for the given genre, or commonPop as default
    	auto it = genrePlans.find(g);
    	return it != genrePlans.end() ? it->second : commonPop;
	}
	// generateSong does all the lifting.
	// ---
	using InstrumentFactory = std::function<std::unique_ptr<Instruments::Instrument>()>; // get the instruments that instrument.h scanned

    template<typename T>
    struct InstrumentRegistrar {
        InstrumentRegistrar(const std::string& name) {
            getInstrumentRegistry()[name] = []() -> std::unique_ptr<Instruments::Instrument> {
                return std::unique_ptr<Instruments::Instrument>(std::make_unique<T>(1.0f).release());
            };
        }
    };
		
	std::tuple<std::string, std::vector<Part>, std::vector<Section>> generateSong(Genre g, long double rootFreq = 440.0L, long double bpm = 0.0L) {
    	static thread_local AudioUtils::RandomGenerator rng; // initialize rng
    	long double totalDur = rng.dist(180.0L, 300.0L); // seconds
    	::SDL_Log("Selected a song duration of %.2Lf seconds", totalDur);
		
		std::string title = generateTitle();

    	// Select scale with weighted random selection
    	const std::vector<std::string> scaleNames = genreScales[g];
    	std::string scaleName = scaleNames.empty() ? "major" : scaleNames[0];
    	if (!scaleNames.empty()) {
        	std::vector<double> weights = genreScaleWeights.count(g) ? genreScaleWeights[g] : std::vector<double>(scaleNames.size(), 1.0 / scaleNames.size());
        	weights.resize(scaleNames.size(), weights.empty() ? 1.0 / scaleNames.size() : weights.back());
        	long double r = rng.dist(0.0L, 1.0L);
        	long double cumulative = 0.0L;
        	for (size_t i = 0; i < weights.size() && i < scaleNames.size(); ++i) {
            	cumulative += weights[i];
            	if (r <= cumulative) {
                	scaleName = scaleNames[i];
                	break;
            	}
        	}
        	::SDL_Log("Selected scale: %s", scaleName.c_str());
    	}

    	// Select section plan
		std::vector<std::vector<std::tuple<std::string, std::string, long double>>> sectionPlan = getSectionPlans(g);
		size_t planIndex = sectionPlan.empty() ? 0 : static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(sectionPlan.size())));
		std::vector<std::tuple<std::string, std::string, long double>> selectedPlan = sectionPlan.empty() ? std::vector<std::tuple<std::string, std::string, long double>>() : sectionPlan[planIndex];
		::SDL_Log("Selected section plan with %zu sections", selectedPlan.size());

		// Extend plan if needed (20% chance to add sections)
		std::vector<std::tuple<std::string, std::string, long double>> extendedPlan = selectedPlan;
		int verseCount = 2, chorusCount = 2, bridgeCount = 0, soloCount = 0;
		if (rng.dist(0, 1) < 0.2) {
    		int extraSections = static_cast<int>(rng.dist(1, 4));
    			for (int i = 0; i < extraSections; ++i) {
        		long double prob = rng.dist(0, 1);
        		std::string name, templateName;
        		long double progress = 0.6 + i * 0.1;
        		if (prob < 0.4) {
           			if (g == Genre::JAZZ || g == Genre::BLUES || g == Genre::CLASSICAL_JAZZ_FUSION) {
               			name = "Head" + std::to_string(++verseCount);
           			} else if (g == Genre::METAL || g == Genre::PUNK || g == Genre::ROCK) {
               			name = "Riff" + std::to_string(++verseCount);
           			} else if (g == Genre::CLASSICAL || g == Genre::NEW_AGE) {
               			name = "Theme" + std::to_string(++verseCount);
           			} else if (g == Genre::FOLK || g == Genre::COUNTRY || g == Genre::BLUEGRASS) {
               			name = "Stanza" + std::to_string(++verseCount);
           			} else if (g == Genre::DISCO || g == Genre::FUNK) {
               			name = "Groove" + std::to_string(++verseCount);
           			} else if (g == Genre::INDIE) {
               			name = "Vibe" + std::to_string(++verseCount);
           			} else {
               			name = "Verse" + std::to_string(++verseCount);
           			}
           			templateName = "Verse";
        		} else if (prob < 0.8) {
           			if (g == Genre::EDM || g == Genre::TECHNO || g == Genre::ELECTRONICA || g == Genre::DUBSTEP) {
               			name = "Drop" + std::to_string(++chorusCount);
           			} else if (g == Genre::HIPHOP || g == Genre::RAP || g == Genre::TRAP) {
               			name = "Hook" + std::to_string(++chorusCount);
           			} else if (g == Genre::POP || g == Genre::RNB || g == Genre::SOUL) {
               			name = "Refrain" + std::to_string(++chorusCount);
           			} else if (g == Genre::REGGAE || g == Genre::REGGAETON) {
               			name = "Rasta" + std::to_string(++chorusCount);
           			} else if (g == Genre::DISCO || g == Genre::FUNK) {
               			name = "Jam" + std::to_string(++chorusCount);
           			} else if (g == Genre::INDIE) {
               				name = "Hook" + std::to_string(++chorusCount);
           			} else {
               			name = "Chorus" + std::to_string(++chorusCount);
           			}
           			templateName = "Chorus";
           			progress += 0.2;
        		} else if (prob < 0.9 && bridgeCount < 1) {
           			if (g == Genre::EDM || g == Genre::TECHNO || g == Genre::ELECTRONICA) {
               			name = "Break" + std::to_string(++bridgeCount);
           			} else if (g == Genre::GOSPEL || g == Genre::SOUL) {
               			name = "CallResponse";
           			} else if (g == Genre::LATIN || g == Genre::REGGAETON) {
               			name = "Puente" + std::to_string(++bridgeCount);
           			} else if (g == Genre::WORLD || g == Genre::AMBIENT) {
               			name = "Interlude" + std::to_string(++bridgeCount);
           			} else if (g == Genre::DISCO || g == Genre::FUNK) {
               			name = "Transition" + std::to_string(++bridgeCount);
           			} else if (g == Genre::INDIE) {
               			name = "Shift" + std::to_string(++bridgeCount);
           			} else {
               			name = "Bridge" + std::to_string(++bridgeCount);
           			}
           			templateName = "Bridge";
           			progress += 0.2;
        		} else {
           			if (g == Genre::JAZZ || g == Genre::BLUES || g == Genre::METAL || g == Genre::ROCK || g == Genre::CLASSICAL_JAZZ_FUSION) {
               			name = "Solo" + std::to_string(++soloCount);
           			} else if (g == Genre::NEW_AGE || g == Genre::AMBIENT) {
               			name = "Chant" + std::to_string(++soloCount);
           			} else if (g == Genre::CLASSICAL) {
               			name = "Cadenza" + std::to_string(++soloCount);
           			} else if (g == Genre::DISCO || g == Genre::FUNK) {
               			name = "Breakdown" + std::to_string(++soloCount);
           			} else if (g == Genre::INDIE) {
               			name = "Jam" + std::to_string(++soloCount);
           			} else if (g == Genre::LATIN || g == Genre::REGGAETON) {
               			name = "Impro" + std::to_string(++soloCount);
           			} else {
               			name = "Verse" + std::to_string(++verseCount);
           			}
           			templateName = (g == Genre::CLASSICAL || g == Genre::LATIN || g == Genre::REGGAETON) ? "Verse" : "Solo";
           			progress += 0.1;
        		}
        		extendedPlan.insert(extendedPlan.end() - 1, {name, templateName, std::min(progress, 0.9L)});
        		::SDL_Log("Added section %s (template: %s, progress: %.2Lf)", name.c_str(), templateName.c_str(), progress);
    		}
		}
	
    	// Generate sections
    	std::vector<Section> sections;
    	long double currentTime = 0.0000000000000L;
		// sometime after Monday June 2, 2025.
    	for (const auto& [name, templateName, progress] : extendedPlan) {
        	long double dur = (name == "Intro" || name == "Outro" || name.find("Coda") != std::string::npos) ? rng.dist(7, 10) :
            	        (name.find("Bridge") != std::string::npos || name.find("Break") != std::string::npos) ? rng.dist(16, 32) :
                	    rng.dist(30, 42);
        sections.push_back(SongGen::Section(name, templateName, currentTime, currentTime + dur));
        currentTime += dur;
        ::SDL_Log("Section %s (template: %s, duration: %.2Lf)", name.c_str(), templateName.c_str(), dur);
    }

    // Adjust final section to match total duration
    if (!sections.empty() && currentTime < totalDur) {
        sections.back().endTime = totalDur;
    }

    // Calculate beat duration
    long double beat = 60.0 / bpm;
    ::SDL_Log("Beat duration: %.2Lf seconds (BPM: %.2Lf)", beat, bpm);

    // Determine intro style (5% chance for vocal-only in specific genres)
    bool vocalOnlyIntro = (g == Genre::GOSPEL || g == Genre::SOUL || g == Genre::POP || g == Genre::RAP || g == Genre::HIPHOP) && rng.dist(0, 0.05);
    ::SDL_Log("Intro style: %s", vocalOnlyIntro ? "Vocal-only" : "Standard");
	
    // Estimate total duration of base plan
    long double basePlanDur = 0.0;
    for (size_t i = 0; i < sectionPlan.size(); ++i) {
        const auto& [name, templateName, progress] = extendedPlan[i];
        // Assign duration based on section type
        long double dur;
		if (name == "Intro" || name == "Outro" || name.find("Coda") != std::string::npos) {
    		dur = rng.dist(7, 10); // Example duration as 7 to 10 seconds for Intro/Outro/Coda
		} else if (name.find("Bridge") != std::string::npos || name.find("Break") != std::string::npos) {
    		dur = rng.dist(16, 32); // Example duration for Bridge/Break
		} else {
    		dur = rng.dist(30, 42); // Default duration
		}
        
        basePlanDur += dur;
    }
    // Extend plan dynamically based on totalDur
	extendedPlan.clear(); // Clear existing contents
	for (const auto& plan : sectionPlan) {
    	extendedPlan.insert(extendedPlan.end(), plan.begin(), plan.end());
	}

    if (totalDur > basePlanDur * 1.2) {
        int extraSections = static_cast<int>(totalDur - basePlanDur);
        int verseCount = 2, chorusCount = 2, bridgeCount = 0, soloCount = 1;
        for (int i = 0; i < extraSections; ++i) {
            long double prob = rng.dist(0.0L, 1.0L);
            if (prob < 0.4L) {
                std::string name = (g == Genre::JAZZ || g == Genre::BLUES) ? "Head" + std::to_string(++verseCount) : 
                                  (g == Genre::METAL || g == Genre::PUNK) ? "Riff" + std::to_string(++verseCount) : 
                                  "Verse" + std::to_string(++verseCount);
                extendedPlan.insert(extendedPlan.end() - 1, {name, "Verse", 0.6f + i * 0.1f});
            } else if (prob < 0.8L) {
                std::string name = (g == Genre::EDM || g == Genre::TECHNO) ? "Drop" + std::to_string(++chorusCount) : 
                                  (g == Genre::HIPHOP || g == Genre::RAP) ? "Hook" + std::to_string(++chorusCount) : 
                                  "Chorus" + std::to_string(++chorusCount);
                extendedPlan.insert(extendedPlan.end() - 1, {name, "Chorus", 0.8f + i * 0.1f});
            } else if (prob < 0.9L && bridgeCount < 1) {
                std::string name = (g == Genre::EDM || g == Genre::TECHNO) ? "Break" + std::to_string(++bridgeCount) : 
                                  (g == Genre::GOSPEL || g == Genre::SOUL) ? "CallResponse" : 
                                  "Bridge" + std::to_string(++bridgeCount);
                extendedPlan.insert(extendedPlan.end() - 1, {name, "Bridge", 0.85f + i * 0.1f});
            } else {
                std::string name = (g == Genre::JAZZ || g == Genre::BLUES || g == Genre::METAL || g == Genre::ROCK) ? "Solo" + std::to_string(++soloCount) : 
                                  "Verse" + std::to_string(++verseCount);
                extendedPlan.insert(extendedPlan.end() - 1, {name, "Verse", 0.7f + i * 0.1f});
            }
        }
    }
    // Generate sections
    for (size_t i = 0; i < extendedPlan.size(); ++i) {
        const auto& [name, templateName, progress] = extendedPlan[i];
        long double endTime = progress;
        if (endTime > totalDur) endTime = totalDur;
        sections.emplace_back(name, endTime, progress, templateName);        
    }
    // Adjust final section to exactly match totalDur
    if (!sections.empty() && sections.back().endTime < totalDur) {
        sections.back().endTime = totalDur;
        ::SDL_Log("Adjusted final section %s end time to %.2Lf seconds", sections.back().name.c_str(), totalDur);
    }
	
    std::map<std::string, std::vector<std::string>> sectionInstruments;
    const auto& availableInstruments = getInstruments.at(g); // Retrieve set of instruments for genre g
    std::vector<std::string> instrumentVec(availableInstruments.begin(), availableInstruments.end()); // Convert set to vector for random access

    for (const auto& section : sections) {
        std::vector<std::string> insts;
        if (section.name == "Intro" && vocalOnlyIntro) {
            // Binary choice for vocal in intro
            insts.push_back(static_cast<int>(rng.dist(0.0L, 2.0L) >= 1.0L) ? "vocal_0" : "vocal_1");
        } else {
            // Base instruments for all sections
            if (!instrumentVec.empty()) {
                size_t idx = static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(instrumentVec.size())));
                insts.push_back(instrumentVec[idx]); // Melody-like
            }

            // Add genre-specific instruments
            if (section.templateName == "Chorus") {
                if (g == Genre::EDM || g == Genre::TECHNO || g == Genre::AMBIENT || g == Genre::DUBSTEP || g == Genre::ELECTRONICA || g == Genre::TRAP) {
                    insts.push_back("subbass");
                }
                if (g == Genre::CLASSICAL || g == Genre::AMBIENT || g == Genre::GOSPEL || g == Genre::NEW_AGE || g == Genre::CLASSICAL_JAZZ_FUSION) {
                    insts.push_back("pad");
                }
                if (!instrumentVec.empty()) {
                    size_t idx = static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(instrumentVec.size())));
                    insts.push_back(instrumentVec[idx]); // Extra for chorus
                }
            } else if (section.templateName == "Verse" || section.templateName == "Solo" || section.templateName == "Head") {
                if (g == Genre::ROCK || g == Genre::PUNK || g == Genre::METAL || g == Genre::COUNTRY || g == Genre::FOLK || g == Genre::REGGAE || g == Genre::BLUEGRASS || g == Genre::INDIE) {
                    insts.push_back("guitar");
                }
                if (g == Genre::JAZZ || g == Genre::BLUES || g == Genre::CLASSICAL_JAZZ_FUSION) {
                    insts.push_back("saxophone");
                }
            }

            // Add percussion for non-intro sections
            if (section.name != "Intro" && (
                g == Genre::ROCK || g == Genre::PUNK || g == Genre::METAL || g == Genre::DISCO || g == Genre::FUNK ||
                g == Genre::EDM || g == Genre::TECHNO || g == Genre::LATIN || g == Genre::REGGAE || g == Genre::REGGAETON ||
                g == Genre::TRAP || g == Genre::DUBSTEP || g == Genre::ELECTRONICA || g == Genre::WORLD || g == Genre::BLUEGRASS)) {
                insts.push_back("kick");
                insts.push_back("snare");
                insts.push_back((g == Genre::ROCK || g == Genre::METAL) ? "cymbal" : "hihat_closed");
            } else if (section.name != "Intro" && (g == Genre::JAZZ || g == Genre::BLUES || g == Genre::CLASSICAL_JAZZ_FUSION)) {
                insts.push_back("hihat_closed");
                insts.push_back("snare");
            }

            // Add vocal for specific genres and sections
            if ((g == Genre::RAP || g == Genre::HIPHOP || g == Genre::GOSPEL || g == Genre::SOUL || g == Genre::RNB ||
                g == Genre::REGGAETON || (g == Genre::POP && static_cast<int>(rng.dist(0.0L, 2.0L) >= 1.0L))) &&
                section.templateName != "Intro") {
                insts.push_back(static_cast<int>(rng.dist(0.0L, 2.0L) >= 1.0L) ? "vocal_0" : "vocal_1");
            }
        }

        // Remove duplicates while preserving order
        std::vector<std::string> uniqueInsts;
        std::set<std::string> seen;
        for (const auto& inst : insts) {
            if (seen.insert(inst).second) {
                uniqueInsts.push_back(inst);
            }
        }
        sectionInstruments[section.name] = uniqueInsts;
        ::SDL_Log("Section %s instruments: %s",
                  section.name.c_str(),
                  std::accumulate(uniqueInsts.begin(), uniqueInsts.end(), std::string(),
                      [](const std::string& a, const std::string& b) { return a.empty() ? b : a + ", " + b; }).c_str());
    }

// Generate parts based on section instruments
for (const auto& section : sections) {
    const auto& insts = sectionInstruments[section.name];
    for (const auto& inst : insts) {
        if (inst.find("vocal") != std::string::npos) {
            Part vocal = generateVocal(g, scaleName, rootFreq, totalDur, sections, bpm);
            vocal.instrument = inst;
            vocal.notes.erase(std::remove_if(vocal.notes.begin(), vocal.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                vocal.notes.end());
            parts.push_back(vocal);
        } else if (inst == "bass" || inst == "subbass") {
            Part bass = generateBass(g, scaleName, rootFreq, totalDur, sections, bpm);
            bass.instrument = inst;
            bass.notes.erase(std::remove_if(bass.notes.begin(), bass.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                bass.notes.end());
            parts.push_back(bass);
        } else if (inst == "guitar") {
            Part guitar = generateGuitar(g, scaleName, rootFreq, totalDur, sections, bpm);
            guitar.instrument = inst;
            guitar.notes.erase(std::remove_if(guitar.notes.begin(), guitar.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                guitar.notes.end());
            parts.push_back(guitar);
        } else if (inst == "kick" || inst == "snare" || inst == "cymbal" || inst == "hihat_closed" || inst == "hihat_open" || inst == "clap") {
            Part rhythm = generateRhythm(g, totalDur, beat, bpm, inst, sections);
            rhythm.notes.erase(std::remove_if(rhythm.notes.begin(), rhythm.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                rhythm.notes.end());
            parts.push_back(rhythm);
        } else if (inst == "syntharp" || inst == "leadsynth" || (inst == "piano" && (g == Genre::EDM || g == Genre::TECHNO || g == Genre::CLASSICAL))) {
            Part arp = generateArpeggio(g, scaleName, rootFreq, totalDur, sections, bpm);
            arp.instrument = inst;
            arp.notes.erase(std::remove_if(arp.notes.begin(), arp.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                arp.notes.end());
            parts.push_back(arp);
		} else if (inst == "piano") {
            Part piano = generatePiano(g, scaleName, rootFreq, totalDur, sections, bpm);
            piano.instrument = inst;
            piano.notes.erase(std::remove_if(piano.notes.begin(), piano.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                piano.notes.end());
            parts.push_back(piano);
        } else if (inst == "pad" || inst == "strings" || inst == "organ") {
            Part harmony = generateHarmony(g, scaleName, rootFreq, totalDur, sections, bpm);
            harmony.instrument = inst;
            harmony.notes.erase(std::remove_if(harmony.notes.begin(), harmony.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                harmony.notes.end());
            parts.push_back(harmony);
        } else {
            Part melody = generateMelody(g, scaleName, rootFreq, totalDur, sections, bpm);
            melody.instrument = inst;
            melody.notes.erase(std::remove_if(melody.notes.begin(), melody.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                melody.notes.end());
            parts.push_back(melody);
        }
    }
}

    // Apply volume adjustments
    std::vector<std::string> percussionInstruments = {"kick", "snare", "cymbal", "hihat_closed", "hihat_open", "clap"};
    for (auto& part : parts) {
        // Check if part is percussion
        bool isPercussion = std::find(percussionInstruments.begin(), percussionInstruments.end(), part.instrument) != percussionInstruments.end();

        // Adjust note volumes
        for (auto& note : part.notes) {
            if (isPercussion) {
                note.volume = std::min(1.0L, note.volume * 1.5L); // 150% for percussion
                ::SDL_Log("Increased volume to %.2Lf for percussion note in part %s", note.volume, part.sectionName.c_str());
            }
            note.volume = std::min(1.0L, note.volume * 0.75L); // 75% for all parts
            ::SDL_Log("Reduced volume to %.2Lf for note in part %s", note.volume, part.sectionName.c_str());
        }

        // Adjust volume automation
        for (auto& [time, value] : part.volumeAutomation) {
            if (isPercussion) {
                value = std::min(1.0L, value * 1.5L); // 150% for percussion
                ::SDL_Log("Increased volume automation to %.2Lf at t=%.2Lf for percussion part %s", value, time, part.sectionName.c_str());
            }
            value = std::min(1.0L, value * 0.75L); // 75% for all parts
            ::SDL_Log("Reduced volume automation to %.2Lf at t=%.2Lf for part %s", value, time, part.sectionName.c_str());
        }

        // Apply fade-in and fade-out
        // Fade-in: 0 to 5 seconds, volume from 0 to adjusted initial volume
        long double initialVolume = part.volumeAutomation.empty() ? (isPercussion ? 0.5L * 1.5L * 0.75L : 0.5L * 0.75L) : part.volumeAutomation[0].second;
        part.volumeAutomation.insert(part.volumeAutomation.begin(), {{0.0L, 0.0L}, {5.0L, initialVolume}});

        // Fade-out: last 5 seconds, volume from last value to 0        
        long double lastVolume = initialVolume;
        for (const auto& [time, value] : part.volumeAutomation) {
            if (time <= totalDur && time > totalDur - 5.0L) {
                lastVolume = value;
            }
        }
        part.volumeAutomation.emplace_back(totalDur - 5.0L, lastVolume);
        part.volumeAutomation.emplace_back(totalDur, 0.0L);

        // Sort volume automation to ensure chronological order
        std::sort(part.volumeAutomation.begin(), part.volumeAutomation.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        // Log the number of notes before processing
        ::SDL_Log("Before processing part %s: %zu notes", part.sectionName.c_str(), part.notes.size());

        // Sort and clean notes
        std::sort(part.notes.begin(), part.notes.end(),
                  [](const Note& a, const Note& b) {
                      return std::isfinite(a.startTime) && std::isfinite(b.startTime) ? a.startTime < b.startTime : false;
                  });
        part.notes.erase(
            std::remove_if(part.notes.begin(), part.notes.end(),
                [totalDur](const Note& n) {
                    bool invalid = n.startTime >= totalDur || !std::isfinite(n.freq) ||
                                   !std::isfinite(n.startTime) || !std::isfinite(n.duration);
                    if (invalid) {
                        ::SDL_Log("Removing invalid note: start=%.2Lf, freq=%.2Lf, duration=%.2Lf",
                                  n.startTime, n.freq, n.duration);
                    }
                    return invalid;
                }),
            part.notes.end()
        );

        // Log the number of notes after processing
        ::SDL_Log("After processing part %s: %zu notes", part.sectionName.c_str(), part.notes.size());
    }

    // Check total notes and truncate if necessary
    size_t totalNotes = std::accumulate(parts.begin(), parts.end(), size_t(0),
        [](size_t sum, const Part& p) { return sum + p.notes.size(); });
    if (totalNotes > 5000) {
        ::SDL_Log("Warning: Total notes %zu exceeds safe limit, truncating", totalNotes);
        for (auto& part : parts) {
            if (part.notes.size() > 1000) {
                part.notes.resize(1000);
                ::SDL_Log("Truncated part %s to 1000 notes", part.sectionName.c_str());
            }
        }
        totalNotes = std::accumulate(parts.begin(), parts.end(), size_t(0),
            [](size_t sum, const Part& p) { return sum + p.notes.size(); });
    }

    ::SDL_Log("Song generation complete: %zu parts, %zu sections, total notes %zu",
              parts.size(), sections.size(), totalNotes);

    return {title, parts, sections};
}

void saveToFile(const std::string& title, const std::string& genres, long double bpm, const std::string& scale, long double rootFrequency, long double duration, const std::vector<Part>& parts, const std::vector<Section>& sections, const std::string& filename) {
    ::SDL_Log("Saving song '%s' to file %s", title.c_str(), filename.c_str());
    std::ofstream out(filename);
    if (!out.is_open()) {
        ::SDL_Log("Failed to open file %s for writing", filename.c_str());
        return;
    }

    out << "Song: " << title << "\n";
    out << "Genre: " << genres << "\n";
    out << "BPM: " << bpm << "\n";
    out << "Scale: " << scale << "\n";
    out << "RootFrequency: " << rootFrequency << "\n";
    out << "Duration: " << duration << "\n";

    out << "Sections: " << sections.size() << "\n";
    for (const auto& section : sections) {
        out << "Section: " << section.name << " " << section.startTime << " " << section.endTime
            << " Progress: " << section.progress << " Template: " << section.templateName << "\n";
    }
    out << "Parts: " << parts.size() << "\n";
    for (const auto& part : parts) {
        out << "Part: " << part.sectionName << "\n";
        out << "Instrument: " << part.instrument << "\n";
        out << "Pan: " << part.pan << "\n";
        out << "ReverbMix: " << part.reverbMix << "\n";
        out << "UseReverb: " << (part.useReverb ? "true" : "false") << "\n";
        out << "ReverbDelay: " << part.reverbDelay << "\n";
        out << "ReverbDecay: " << part.reverbDecay << "\n";
        out << "ReverbMixFactor: " << part.reverbMix << "\n";
        out << "UseDistortion: " << (part.useDistortion ? "true" : "false") << "\n";
        out << "DistortionDrive: " << part.distortionDrive << "\n";
        out << "DistortionThreshold: " << part.distortionThreshold << "\n";
        out << "Notes: " << part.notes.size() << "\n";
        for (const auto& note : part.notes) {
            out << "Note: " << note.freq << " " << note.duration << " " << note.startTime
                << " Phoneme: " << note.phoneme << " Open: " << (note.open ? "true" : "false")
                << " Volume: " << note.volume << " Velocity: " << note.velocity << "\n";
        }
        out << "PanAutomation: " << part.panAutomation.size() << "\n";
        for (const auto& [time, value] : part.panAutomation) {
            out << "PanPoint: " << time << " " << value << "\n";
        }
        out << "VolumeAutomation: " << part.volumeAutomation.size() << "\n";
        for (const auto& [time, value] : part.volumeAutomation) {
            out << "VolumePoint: " << time << " " << value << "\n";
        }
        out << "ReverbMixAutomation: " << part.reverbMixAutomation.size() << "\n";
        for (const auto& [time, value] : part.reverbMixAutomation) {
            out << "ReverbMixPoint: " << time << " " << value << "\n";
        }
    }
    out.close();
    ::SDL_Log("Song saved successfully to %s", filename.c_str());
}

private: // songgen.h handles it.
    const long double sampleRate = 44100.0L; // max SDL2 supports
    const std::vector<long double> durations = {
        0.0284091L, 0.0625L, 0.073864L, 0.125L, 0.136364L, 0.147726L, 0.210226L,
        0.25L, 0.272727L, 0.460224L, 0.5L, 0.886364L, 1.0L
    };
	
	// Map of musical scales with interval steps in semitones
	const std::map<std::string, std::vector<long double>> scales = {
    	{"chromatic", {0.0L, 1.0L, 2.0L, 3.0L, 4.0L, 5.0L, 6.0L, 7.0L, 8.0L, 9.0L, 10.0L, 11.0L}},
    	{"dorian", {0.0L, 2.0L, 3.0L, 5.0L, 7.0L, 9.0L, 10.0L}},
    	{"harmonic_minor", {0.0L, 2.0L, 3.0L, 5.0L, 7.0L, 8.0L, 11.0L}},
    	{"lydian", {0.0L, 2.0L, 4.0L, 6.0L, 7.0L, 9.0L, 11.0L}},
    	{"major", {0.0L, 2.0L, 4.0L, 5.0L, 7.0L, 9.0L, 11.0L}},
    	{"minor", {0.0L, 2.0L, 3.0L, 5.0L, 7.0L, 8.0L, 10.0L}},
    	{"mixolydian", {0.0L, 2.0L, 4.0L, 5.0L, 7.0L, 9.0L, 10.0L}},
    	{"pentatonic_major", {0.0L, 2.0L, 4.0L, 7.0L, 9.0L}},
    	{"pentatonic_minor", {0.0L, 3.0L, 5.0L, 7.0L, 10.0L}},
    	{"phrygian", {0.0L, 1.0L, 3.0L, 5.0L, 7.0L, 8.0L, 10.0L}},
    	{"blues", {0.0L, 3.0L, 5.0L, 6.0L, 7.0L, 10.0L}},
    	{"whole_tone", {0.0L, 2.0L, 4.0L, 6.0L, 8.0L, 10.0L}}
	};

	// Map of genres to their associated scales
	std::map<Genre, std::vector<std::string>> genreScales = {
    	{AMBIENT, {"minor", "dorian", "major", "whole_tone"}},
    	{BLUEGRASS, {"major", "pentatonic_major", "pentatonic_minor"}},
    	{BLUES, {"blues", "pentatonic_minor"}},
    	{CLASSICAL, {"major", "minor", "harmonic_minor", "phrygian"}},
    	{CLASSICAL_JAZZ_FUSION, {"dorian", "mixolydian", "harmonic_minor"}},
    	{COUNTRY, {"major", "pentatonic_major"}},
    	{DISCO, {"major", "minor"}},
    	{DUBSTEP, {"minor", "pentatonic_minor"}},
    	{EDM, {"minor", "pentatonic_minor", "major"}},
    	{ELECTRONICA, {"minor", "whole_tone", "pentatonic_minor"}},
    	{FOLK, {"major", "minor", "dorian"}},
    	{FUNK, {"minor", "pentatonic_minor", "dorian"}},
    	{GOSPEL, {"major", "blues", "pentatonic_major"}},
    	{HIPHOP, {"minor", "pentatonic_minor", "blues"}},
    	{INDIE, {"major", "minor", "dorian"}},
    	{JAZZ, {"dorian", "mixolydian", "blues", "chromatic"}},
    	{LATIN, {"major", "minor", "dorian"}},
    	{METAL, {"minor", "harmonic_minor", "pentatonic_minor", "phrygian"}},
    	{NEW_AGE, {"major", "minor", "whole_tone"}},
    	{POP, {"major", "pentatonic_major", "minor"}},
    	{PUNK, {"major", "minor"}},
    	{RAP, {"minor", "pentatonic_minor"}},
    	{REGGAE, {"minor", "dorian"}},
    	{REGGAETON, {"major", "minor", "dorian"}},
    	{RNB, {"major", "minor", "blues"}},
    	{ROCK, {"major", "minor", "pentatonic_minor", "blues"}},
    	{SOUL, {"major", "minor", "blues"}},
    	{TECHNO, {"minor", "pentatonic_minor", "whole_tone"}},
    	{TRAP, {"minor", "pentatonic_minor", "blues"}},
    	{WORLD, {"major", "minor", "dorian", "harmonic_minor"}}
	};
	
	// Map of genres to scale selection weights, replacing switch case
std::map<Genre, std::vector<double>> genreScaleWeights = {
    	{AMBIENT, {0.30, 0.30, 0.20, 0.20}}, // minor, dorian, major, whole_tone
    	{BLUEGRASS, {0.50, 0.30, 0.20}}, // major, pentatonic_major, pentatonic_minor
    	{BLUES, {0.70, 0.30}}, // blues, pentatonic_minor
    	{CLASSICAL, {0.40, 0.30, 0.15, 0.15}}, // major, minor, harmonic_minor, phrygian
    	{CLASSICAL_JAZZ_FUSION, {0.40, 0.30, 0.30}}, // dorian, mixolydian, harmonic_minor
    	{COUNTRY, {0.70, 0.30}}, // major, pentatonic_major
    	{DISCO, {0.60, 0.40}}, // major, minor
    	{DUBSTEP, {0.60, 0.40}}, // minor, pentatonic_minor
    	{EDM, {0.50, 0.30, 0.20}}, // minor, pentatonic_minor, major
    	{ELECTRONICA, {0.50, 0.30, 0.20}}, // minor, whole_tone, pentatonic_minor
    	{FOLK, {0.50, 0.30, 0.20}}, // major, minor, dorian
    	{FUNK, {0.50, 0.30, 0.20}}, // minor, pentatonic_minor, dorian
    	{GOSPEL, {0.50, 0.30, 0.20}}, // major, blues, pentatonic_major
    	{HIPHOP, {0.50, 0.30, 0.20}}, // minor, pentatonic_minor, blues
    	{INDIE, {0.50, 0.30, 0.20}}, // major, minor, dorian
    	{JAZZ, {0.30, 0.25, 0.20, 0.25}}, // dorian, mixolydian, blues, chromatic
    	{LATIN, {0.50, 0.30, 0.20}}, // major, minor, dorian
    	{METAL, {0.40, 0.30, 0.20, 0.10}}, // minor, harmonic_minor, pentatonic_minor, phrygian
    	{NEW_AGE, {0.40, 0.30, 0.30}}, // major, minor, whole_tone
    	{POP, {0.50, 0.30, 0.20}}, // major, pentatonic_major, minor
    	{PUNK, {0.60, 0.40}}, // major, minor
    	{RAP, {0.60, 0.40}}, // minor, pentatonic_minor
    	{REGGAE, {0.60, 0.40}}, // minor, dorian
    	{REGGAETON, {0.50, 0.30, 0.20}}, // major, minor, dorian
    	{RNB, {0.50, 0.30, 0.20}}, // major, minor, blues
    	{ROCK, {0.40, 0.25, 0.20, 0.15}}, // major, minor, pentatonic_minor, blues
    	{SOUL, {0.50, 0.30, 0.20}}, // major, minor, blues
    	{TECHNO, {0.50, 0.30, 0.20}}, // minor, pentatonic_minor, whole_tone
    	{TRAP, {0.50, 0.30, 0.20}}, // minor, pentatonic_minor, blues
    	{WORLD, {0.40, 0.30, 0.20, 0.10}} // major, minor, dorian, harmonic_minor
	};

    // Dynamic instrument scanning
    std::vector<std::string> getAvailableInstruments() {
        std::vector<std::string> instruments;
        for (const auto& entry : std::filesystem::directory_iterator("../instrument/")) {
            if (entry.path().extension() == ".h") {
                std::string instrument = entry.path().stem().string();
                instruments.push_back(instrument);
            }
        }
        return instruments;
    }

    // Base weights for each genre and instrument (0.0L to 1.0L)
    std::map<Genre, std::map<std::string, long double>> genreInstrumentBaseWeights = {
        {CLASSICAL, {{"violin", 0.95L}, {"cello", 0.9L}, {"flute", 0.8L}, {"piano", 0.85L}, {"trumpet", 0.7L}, {"organ", 0.6L}, {"oboe", 0.5L}, {"clarinet", 0.5L}, {"tuba", 0.3L}}},
        {JAZZ, {{"piano", 0.9L}, {"saxophone", 0.85L}, {"trumpet", 0.8L}, {"bass", 0.9L}, {"hihat", 0.7L}, {"snare", 0.65L}, {"cymbal", 0.6L}}},
        {POP, {{"guitar", 0.8L}, {"bass", 0.85L}, {"piano", 0.7L}, {"kick", 0.9L}, {"snare", 0.85L}, {"syntharp", 0.6L}, {"leadsynth", 0.5L}, {"vocal", 0.7L}}},
        {ROCK, {{"guitar", 0.95L}, {"bass", 0.95L}, {"kick", 0.9L}, {"snare", 0.85L}, {"cymbal", 0.8L}, {"leadsynth", 0.4L}}},
        {TECHNO, {{"kick", 0.95L}, {"hihat", 0.9L}, {"syntharp", 0.85L}, {"subbass", 0.9L}, {"leadsynth", 0.7L}, {"pad", 0.6L}}},
        {RAP, {{"kick", 0.95L}, {"snare", 0.9L}, {"hihat", 0.85L}, {"bass", 0.9L}, {"vocal", 0.95L}, {"syntharp", 0.5L}}},
        {BLUES, {{"guitar", 0.9L}, {"bass", 0.85L}, {"hihat", 0.7L}, {"snare", 0.65L}, {"piano", 0.7L}, {"saxophone", 0.6L}}},
        {COUNTRY, {{"guitar", 0.9L}, {"bass", 0.8L}, {"kick", 0.7L}, {"snare", 0.65L}, {"steelguitar", 0.85L}, {"violin", 0.6L}, {"piano", 0.5L}, {"banjo", 0.4L}}},
        {FOLK, {{"guitar", 0.9L}, {"violin", 0.7L}, {"flute", 0.6L}, {"sitar", 0.5L}, {"marimba", 0.5L}, {"banjo", 0.4L}}},
        {REGGAE, {{"bass", 0.95L}, {"guitar", 0.8L}, {"kick", 0.85L}, {"hihat", 0.7L}, {"piano", 0.6L}, {"organ", 0.65L}}},
        {METAL, {{"guitar", 0.95L}, {"bass", 0.95L}, {"kick", 0.9L}, {"snare", 0.85L}, {"cymbal", 0.8L}, {"leadsynth", 0.4L}}},
        {PUNK, {{"guitar", 0.95L}, {"bass", 0.9L}, {"kick", 0.85L}, {"snare", 0.8L}, {"cymbal", 0.75L}}},
        {DISCO, {{"bass", 0.9L}, {"guitar", 0.85L}, {"kick", 0.95L}, {"hihat", 0.8L}, {"clap", 0.85L}, {"syntharp", 0.7L}}},
        {FUNK, {{"bass", 0.95L}, {"guitar", 0.9L}, {"kick", 0.85L}, {"snare", 0.8L}, {"hihat", 0.75L}, {"saxophone", 0.6L}}},
        {SOUL, {{"piano", 0.9L}, {"bass", 0.85L}, {"kick", 0.8L}, {"snare", 0.75L}, {"guitar", 0.7L}, {"saxophone", 0.6L}, {"vocal", 0.85L}}},
        {GOSPEL, {{"piano", 0.9L}, {"bass", 0.8L}, {"kick", 0.75L}, {"snare", 0.7L}, {"vocal", 0.95L}, {"organ", 0.85L}}},
        {AMBIENT, {{"pad", 0.9L}, {"piano", 0.7L}, {"subbass", 0.65L}, {"leadsynth", 0.6L}, {"flute", 0.5L}}},
        {EDM, {{"kick", 0.95L}, {"hihat", 0.9L}, {"syntharp", 0.85L}, {"subbass", 0.9L}, {"leadsynth", 0.7L}, {"pad", 0.6L}}},
        {LATIN, {{"guitar", 0.8L}, {"bass", 0.85L}, {"kick", 0.7L}, {"clap", 0.75L}, {"marimba", 0.7L}, {"trumpet", 0.6L}}},
        {HIPHOP, {{"kick", 0.95L}, {"snare", 0.9L}, {"hihat", 0.85L}, {"bass", 0.9L}, {"vocal", 0.9L}, {"syntharp", 0.6L}}},
        {WORLD, {{"sitar", 0.8L}, {"flute", 0.7L}, {"marimba", 0.7L}, {"guitar", 0.6L}, {"tambourine", 0.6L}, {"oboe", 0.5L}}},
        {RNB, {{"piano", 0.9L}, {"bass", 0.85L}, {"kick", 0.8L}, {"snare", 0.75L}, {"guitar", 0.7L}, {"vocal", 0.9L}, {"syntharp", 0.6L}}},
        {INDIE, {{"guitar", 0.9L}, {"bass", 0.85L}, {"kick", 0.75L}, {"snare", 0.7L}, {"piano", 0.65L}, {"syntharp", 0.5L}}},
        {ELECTRONICA, {{"syntharp", 0.9L}, {"subbass", 0.85L}, {"leadsynth", 0.8L}, {"pad", 0.75L}, {"kick", 0.7L}, {"hihat", 0.65L}}},
        {DUBSTEP, {{"subbass", 0.95L}, {"kick", 0.9L}, {"snare", 0.85L}, {"hihat", 0.8L}, {"syntharp", 0.75L}, {"leadsynth", 0.6L}}},
        {CLASSICAL_JAZZ_FUSION, {{"piano", 0.9L}, {"saxophone", 0.85L}, {"violin", 0.8L}, {"bass", 0.8L}, {"flute", 0.7L}, {"trumpet", 0.65L}}},
        {REGGAETON, {{"kick", 0.9L}, {"clap", 0.85L}, {"bass", 0.8L}, {"syntharp", 0.75L}, {"marimba", 0.7L}, {"vocal", 0.8L}}},
        {BLUEGRASS, {{"banjo", 0.95L}, {"guitar", 0.9L}, {"violin", 0.85L}, {"bass", 0.8L}, {"xylophone", 0.5L}}},
        {TRAP, {{"kick", 0.95L}, {"snare", 0.9L}, {"hihat", 0.85L}, {"subbass", 0.95L}, {"syntharp", 0.7L}, {"vocal", 0.8L}}},
        {NEW_AGE, {{"pad", 0.9L}, {"piano", 0.8L}, {"flute", 0.7L}, {"syntharp", 0.6L}, {"subbass", 0.5L}}}
    };

    // Conditional weights: Adjusts probability of selecting an instrument if another is already selected
    std::map<Genre, std::map<std::string, std::map<std::string, long double>>> genreInstrumentConditionalWeights = {
        {CLASSICAL, {
            {"violin", {{"cello", 0.3L}, {"flute", 0.2L}, {"piano", 0.2L}}}, // If violin selected, boost cello/flute/piano
            {"piano", {{"violin", 0.2L}, {"organ", 0.2L}}}, // If piano selected, boost violin/organ
        }},
        {JAZZ, {
            {"bass", {{"piano", 0.3L}, {"saxophone", 0.2L}, {"trumpet", 0.2L}}}, // Bass boosts piano/sax/trumpet
            {"piano", {{"saxophone", 0.2L}, {"trumpet", 0.2L}}}, // Piano boosts sax/trumpet
        }},
        {POP, {
            {"bass", {{"guitar", 0.3L}, {"kick", 0.2L}, {"snare", 0.2L}}}, // Bass boosts guitar/drums
            {"guitar", {{"bass", 0.3L}, {"leadsynth", 0.2L}}}, // Guitar boosts bass/leadsynth
            {"vocal", {{"piano", 0.2L}, {"syntharp", 0.2L}}}, // Vocal boosts piano/syntharp
        }},
        {ROCK, {
            {"bass", {{"guitar", 0.4L}, {"kick", 0.3L}, {"snare", 0.3L}}}, // Bass strongly boosts guitar/drums
            {"guitar", {{"bass", 0.4L}, {"cymbal", 0.2L}}}, // Guitar boosts bass/cymbal
        }},
        {TECHNO, {
            {"kick", {{"hihat", 0.3L}, {"subbass", 0.3L}, {"syntharp", 0.2L}}}, // Kick boosts hihat/subbass/syntharp
            {"subbass", {{"syntharp", 0.2L}, {"leadsynth", 0.2L}}}, // Subbass boosts syntharp/leadsynth
        }},
        {RAP, {
            {"bass", {{"kick", 0.3L}, {"snare", 0.3L}, {"vocal", 0.2L}}}, // Bass boosts drums/vocal
            {"vocal", {{"syntharp", 0.2L}, {"hihat", 0.2L}}}, // Vocal boosts syntharp/hihat
        }},
        {BLUES, {
            {"guitar", {{"bass", 0.3L}, {"piano", 0.2L}, {"saxophone", 0.2L}}}, // Guitar boosts bass/piano/sax
            {"bass", {{"guitar", 0.3L}, {"snare", 0.2L}}}, // Bass boosts guitar/snare
        }},
        {COUNTRY, {
            {"guitar", {{"bass", 0.3L}, {"steelguitar", 0.3L}, {"banjo", 0.2L}}}, // Guitar boosts bass/steelguitar/banjo
            {"bass", {{"guitar", 0.3L}, {"kick", 0.2L}}}, // Bass boosts guitar/kick
        }},
        {FOLK, {
            {"guitar", {{"violin", 0.3L}, {"flute", 0.2L}, {"sitar", 0.2L}}}, // Guitar boosts violin/flute/sitar
            {"violin", {{"guitar", 0.2L}, {"marimba", 0.2L}}}, // Violin boosts guitar/marimba
        }},
        {REGGAE, {
            {"bass", {{"guitar", 0.3L}, {"kick", 0.3L}, {"organ", 0.2L}}}, // Bass boosts guitar/kick/organ
            {"guitar", {{"bass", 0.3L}, {"hihat", 0.2L}}}, // Guitar boosts bass/hihat
        }},
        {METAL, {
            {"guitar", {{"bass", 0.4L}, {"kick", 0.3L}, {"cymbal", 0.2L}}}, // Guitar strongly boosts bass/drums
            {"bass", {{"guitar", 0.4L}, {"snare", 0.3L}}}, // Bass boosts guitar/snare
        }},
        {PUNK, {
            {"guitar", {{"bass", 0.4L}, {"kick", 0.3L}, {"snare", 0.3L}}}, // Guitar boosts bass/drums
            {"bass", {{"guitar", 0.4L}, {"cymbal", 0.2L}}}, // Bass boosts guitar/cymbal
        }},
        {DISCO, {
            {"bass", {{"guitar", 0.3L}, {"kick", 0.3L}, {"clap", 0.2L}}}, // Bass boosts guitar/kick/clap
            {"kick", {{"hihat", 0.2L}, {"syntharp", 0.2L}}}, // Kick boosts hihat/syntharp
        }},
        {FUNK, {
            {"bass", {{"guitar", 0.4L}, {"kick", 0.3L}, {"saxophone", 0.2L}}}, // Bass boosts guitar/kick/sax
            {"guitar", {{"bass", 0.4L}, {"hihat", 0.2L}}}, // Guitar boosts bass/hihat
        }},
        {SOUL, {
            {"piano", {{"bass", 0.3L}, {"vocal", 0.3L}, {"saxophone", 0.2L}}}, // Piano boosts bass/vocal/sax
            {"vocal", {{"piano", 0.3L}, {"guitar", 0.2L}}}, // Vocal boosts piano/guitar
        }},
        {GOSPEL, {
            {"piano", {{"vocal", 0.3L}, {"organ", 0.3L}, {"bass", 0.2L}}}, // Piano boosts vocal/organ/bass
            {"vocal", {{"piano", 0.3L}, {"organ", 0.2L}}}, // Vocal boosts piano/organ
        }},
        {AMBIENT, {
            {"pad", {{"piano", 0.2L}, {"subbass", 0.2L}, {"flute", 0.2L}}}, // Pad boosts piano/subbass/flute
            {"subbass", {{"leadsynth", 0.2L}, {"pad", 0.2L}}}, // Subbass boosts leadsynth/pad
        }},
        {EDM, {
            {"kick", {{"hihat", 0.3L}, {"subbass", 0.3L}, {"syntharp", 0.2L}}}, // Kick boosts hihat/subbass/syntharp
            {"subbass", {{"leadsynth", 0.2L}, {"pad", 0.2L}}}, // Subbass boosts leadsynth/pad
        }},
        {LATIN, {
            {"bass", {{"guitar", 0.3L}, {"marimba", 0.2L}, {"trumpet", 0.2L}}}, // Bass boosts guitar/marimba/trumpet
            {"kick", {{"clap", 0.3L}, {"marimba", 0.2L}}}, // Kick boosts clap/marimba
        }},
        {HIPHOP, {
            {"bass", {{"kick", 0.3L}, {"snare", 0.3L}, {"vocal", 0.2L}}}, // Bass boosts drums/vocal
            {"vocal", {{"syntharp", 0.2L}, {"hihat", 0.2L}}}, // Vocal boosts syntharp/hihat
        }},
        {WORLD, {
            {"sitar", {{"flute", 0.2L}, {"marimba", 0.2L}, {"tambourine", 0.2L}}}, // Sitar boosts flute/marimba/tambourine
            {"marimba", {{"guitar", 0.2L}, {"oboe", 0.2L}}}, // Marimba boosts guitar/oboe
        }},
        {RNB, {
            {"piano", {{"bass", 0.3L}, {"vocal", 0.3L}, {"guitar", 0.2L}}}, // Piano boosts bass/vocal/guitar
            {"vocal", {{"syntharp", 0.2L}, {"piano", 0.2L}}}, // Vocal boosts syntharp/piano
        }},
        {INDIE, {
            {"guitar", {{"bass", 0.3L}, {"kick", 0.2L}, {"piano", 0.2L}}}, // Guitar boosts bass/kick/piano
            {"bass", {{"guitar", 0.3L}, {"snare", 0.2L}}}, // Bass boosts guitar/snare
        }},
        {ELECTRONICA, {
            {"subbass", {{"syntharp", 0.3L}, {"leadsynth", 0.2L}, {"pad", 0.2L}}}, // Subbass boosts syntharp/leadsynth/pad
            {"kick", {{"hihat", 0.2L}, {"syntharp", 0.2L}}}, // Kick boosts hihat/syntharp
        }},
        {DUBSTEP, {
            {"subbass", {{"kick", 0.3L}, {"snare", 0.3L}, {"syntharp", 0.2L}}}, // Subbass boosts drums/syntharp
            {"kick", {{"hihat", 0.2L}, {"leadsynth", 0.2L}}}, // Kick boosts hihat/leadsynth
        }},
        {CLASSICAL_JAZZ_FUSION, {
            {"piano", {{"saxophone", 0.3L}, {"violin", 0.2L}, {"bass", 0.2L}}}, // Piano boosts sax/violin/bass
            {"saxophone", {{"trumpet", 0.2L}, {"flute", 0.2L}}}, // Saxophone boosts trumpet/flute
        }},
        {REGGAETON, {
            {"kick", {{"clap", 0.3L}, {"bass", 0.2L}, {"marimba", 0.2L}}}, // Kick boosts clap/bass/marimba
            {"bass", {{"syntharp", 0.2L}, {"vocal", 0.2L}}}, // Bass boosts syntharp/vocal
        }},
        {BLUEGRASS, {
            {"banjo", {{"guitar", 0.3L}, {"violin", 0.3L}, {"bass", 0.2L}}}, // Banjo boosts guitar/violin/bass
            {"guitar", {{"banjo", 0.3L}, {"xylophone", 0.2L}}}, // Guitar boosts banjo/xylophone
        }},
        {TRAP, {
            {"subbass", {{"kick", 0.3L}, {"snare", 0.3L}, {"hihat", 0.2L}}}, // Subbass boosts drums
            {"vocal", {{"syntharp", 0.2L}, {"hihat", 0.2L}}}, // Vocal boosts syntharp/hihat
        }},
        {NEW_AGE, {
            {"pad", {{"piano", 0.2L}, {"flute", 0.2L}, {"subbass", 0.2L}}}, // Pad boosts piano/flute/subbass
            {"piano", {{"syntharp", 0.2L}, {"flute", 0.2L}}}, // Piano boosts syntharp/flute
        }}
    };

    // Function to select instruments for a given genre using RNG and conditional weights
    std::vector<std::string> selectInstruments(Genre genre) {
		static thread_local AudioUtils::RandomGenerator rng;
        std::vector<std::string> selectedInstruments;        
        std::map<std::string, long double> currentWeights = genreInstrumentBaseWeights[genre];

        // Get all available instruments from the folder
        std::vector<std::string> availableInstruments = getAvailableInstruments();
        const long double defaultWeight = 0.1L; // Default for unrecognized instruments

        // Ensure all available instruments have a base weight (default if not specified)
        for (const auto& instrument : availableInstruments) {
            if (currentWeights.find(instrument) == currentWeights.end()) {
                currentWeights[instrument] = defaultWeight;
            }
        }
		
        const int maxInstruments = MAX_INSTRUMENTS;
        while (selectedInstruments.size() < maxInstruments) {
            // Break if no instruments have high enough weights
            bool anySelected = false;
            for (const auto& [instrument, weight] : currentWeights) {
                if (rng.dist(0,1) < weight) {
                    selectedInstruments.push_back(instrument);
                    anySelected = true;

                    // Apply conditional weight adjustments
                    if (genreInstrumentConditionalWeights[genre].find(instrument) != genreInstrumentConditionalWeights[genre].end()) {
                        for (const auto& [otherInstrument, weightIncrease] : genreInstrumentConditionalWeights[genre][instrument]) {
                            if (currentWeights.find(otherInstrument) != currentWeights.end()) {
                                currentWeights[otherInstrument] = std::min(1.0L, currentWeights[otherInstrument] + weightIncrease);
                            }
                        }
                    }
                    // Remove selected instrument to avoid re-selection
                    currentWeights.erase(instrument);
                    break; // Re-evaluate weights for remaining instruments
                }
            }
            if (!anySelected || currentWeights.empty()) {
                break; // No more instruments to select
            }
        }

        // Ensure at least one instrument is selected
        if (selectedInstruments.empty() && !currentWeights.empty()) {
            // Select the instrument with the highest base weight
            auto maxWeightIt = std::max_element(currentWeights.begin(), currentWeights.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            selectedInstruments.push_back(maxWeightIt->first);
        }

        return selectedInstruments;
    }

	// Map of genre-specific BPM ranges using RandomGenerator::dist
	std::map<Genre, std::pair<long double, long double>> genreBPM = {
    	{CLASSICAL, {60.0L, 120.0L}},
    	{JAZZ, {80.0L, 160.0L}},
    	{POP, {100.0L, 140.0L}},
    	{ROCK, {90.0L, 160.0L}},
    	{TECHNO, {120.0L, 150.0L}},
    	{RAP, {80.0L, 110.0L}},
    	{BLUES, {60.0L, 120.0L}},
    	{COUNTRY, {90.0L, 130.0L}},
    	{FOLK, {80.0L, 120.0L}},
    	{REGGAE, {60.0L, 90.0L}},
    	{METAL, {100.0L, 180.0L}},
    	{PUNK, {140.0L, 200.0L}},
    	{DISCO, {110.0L, 130.0L}},
    	{FUNK, {90.0L, 120.0L}},
    	{SOUL, {80.0L, 120.0L}},
    	{GOSPEL, {70.0L, 110.0L}},
    	{AMBIENT, {50.0L, 90.0L}},
    	{EDM, {120.0L, 140.0L}},
    	{LATIN, {90.0L, 130.0L}},
    	{HIPHOP, {80.0L, 110.0L}},
    	{WORLD, {70.0L, 120.0L}},
    	{RNB, {80.0L, 120.0L}},
    	{INDIE, {90.0L, 140.0L}},
    	{ELECTRONICA, {110.0L, 140.0L}},
    	{DUBSTEP, {120.0L, 150.0L}},
    	{CLASSICAL_JAZZ_FUSION, {80.0L, 140.0L}},
    	{REGGAETON, {90.0L, 110.0L}},
    	{BLUEGRASS, {90.0L, 140.0L}},
    	{TRAP, {70.0L, 100.0L}},
    	{NEW_AGE, {50.0L, 90.0L}}
	};

	const std::map<Genre, std::string> genreNames = {
    	{CLASSICAL, "Classical"}, {JAZZ, "Jazz"}, {POP, "Pop"}, {ROCK, "Rock"}, {TECHNO, "Techno"},
    	{RAP, "Rap"}, {BLUES, "Blues"}, {COUNTRY, "Country"}, {FOLK, "Folk"}, {REGGAE, "Reggae"},
    	{METAL, "Metal"}, {PUNK, "Punk"}, {DISCO, "Disco"}, {FUNK, "Funk"}, {SOUL, "Soul"},
    	{GOSPEL, "Gospel"}, {AMBIENT, "Ambient"}, {EDM, "EDM"}, {LATIN, "Latin"}, {HIPHOP, "Hip-Hop"},
    	{WORLD, "World"}, {RNB, "R&B"}, {INDIE, "Indie"}, {ELECTRONICA, "Electronica"},
    	{DUBSTEP, "Dubstep"}, {CLASSICAL_JAZZ_FUSION, "Classical-Jazz Fusion"},
    	{REGGAETON, "Reggaeton"}, {BLUEGRASS, "Bluegrass"}, {TRAP, "Trap"}, {NEW_AGE, "New Age"}
	};

	std::map<Genre, std::vector<long double>> genreDurationWeights = {
    	{CLASSICAL, {0.00L, 0.00L, 0.00L, 0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.15L, 0.20L, 0.20L, 0.17L}}, // Longer durations
    	{JAZZ, {0.05L, 0.10L, 0.10L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{POP, {0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.12L, 0.15L, 0.12L, 0.10L, 0.08L, 0.05L, 0.05L}}, // 240–360s
    	{ROCK, {0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.12L, 0.15L, 0.12L, 0.10L, 0.08L, 0.05L, 0.05L}}, // 240–360s
    	{TECHNO, {0.20L, 0.25L, 0.25L, 0.15L, 0.10L, 0.05L, 0.00L, 0.00L, 0.00L, 0.00L, 0.00L, 0.00L, 0.00L}}, // 120–240s
    	{RAP, {0.05L, 0.10L, 0.10L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{BLUES, {0.05L, 0.10L, 0.10L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{COUNTRY, {0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.12L, 0.15L, 0.12L, 0.10L, 0.08L, 0.05L, 0.05L}}, // 240–360s
    	{FOLK, {0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.12L, 0.15L, 0.12L, 0.10L, 0.08L, 0.05L, 0.05L}}, // 240–360s
    	{REGGAE, {0.05L, 0.10L, 0.10L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{METAL, {0.05L, 0.10L, 0.10L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{PUNK, {0.05L, 0.10L, 0.10L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{DISCO, {0.10L, 0.15L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L, 0.00L}}, // 120–300s
    	{FUNK, {0.05L, 0.10L, 0.10L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{SOUL, {0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.12L, 0.15L, 0.12L, 0.10L, 0.08L, 0.05L, 0.05L}}, // 240–360s
    	{GOSPEL, {0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.12L, 0.15L, 0.12L, 0.10L, 0.08L, 0.05L, 0.05L}}, // 240–360s
    	{AMBIENT, {0.00L, 0.00L, 0.00L, 0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.15L, 0.20L, 0.20L, 0.17L}}, // Longer durations
    	{EDM, {0.20L, 0.25L, 0.25L, 0.15L, 0.10L, 0.05L, 0.00L, 0.00L, 0.00L, 0.00L, 0.00L, 0.00L, 0.00L}}, // 120–240s
    	{LATIN, {0.05L, 0.10L, 0.10L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{HIPHOP, {0.05L, 0.10L, 0.10L, 0.15L, 0.15L, 0.10L, 0.10L, 0.10L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{WORLD, {0.03L, 0.05L, 0.08L, 0.12L, 0.15L, 0.12L, 0.10L, 0.10L, 0.08L, 0.07L, 0.05L, 0.03L, 0.02L}}, // 180–420s
    	{RNB, {0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.12L, 0.15L, 0.15L, 0.10L, 0.08L, 0.05L, 0.03L, 0.02L}}, // 240–360s
    	{INDIE, {0.03L, 0.05L, 0.08L, 0.12L, 0.15L, 0.15L, 0.12L, 0.10L, 0.08L, 0.07L, 0.05L, 0.03L, 0.02L}}, // 180–420s
    	{ELECTRONICA, {0.15L, 0.20L, 0.20L, 0.15L, 0.10L, 0.08L, 0.05L, 0.03L, 0.02L, 0.01L, 0.00L, 0.00L, 0.00L}}, // 120–300s
    	{DUBSTEP, {0.10L, 0.15L, 0.20L, 0.15L, 0.12L, 0.10L, 0.08L, 0.05L, 0.03L, 0.02L, 0.00L, 0.00L, 0.00L}}, // 120–300s
    	{CLASSICAL_JAZZ_FUSION, {0.00L, 0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.12L, 0.15L, 0.15L, 0.15L, 0.10L, 0.05L}}, // 300–600s
    	{REGGAETON, {0.05L, 0.10L, 0.12L, 0.15L, 0.15L, 0.10L, 0.10L, 0.08L, 0.05L, 0.05L, 0.03L, 0.02L, 0.00L}}, // 180–360s
    	{BLUEGRASS, {0.02L, 0.03L, 0.05L, 0.08L, 0.10L, 0.12L, 0.15L, 0.15L, 0.12L, 0.10L, 0.08L, 0.05L, 0.00L}}, // 240–360s
    	{TRAP, {0.05L, 0.10L, 0.12L, 0.15L, 0.15L, 0.10L, 0.10L, 0.08L, 0.05L, 0.05L, 0.05L, 0.00L, 0.00L}}, // 180–360s
    	{NEW_AGE, {0.00L, 0.00L, 0.01L, 0.02L, 0.03L, 0.05L, 0.07L, 0.10L, 0.12L, 0.15L, 0.20L, 0.15L, 0.15L}} // Longer durations
	};

	const std::vector<long double> availableFreqs = {
    	27.50L, 29.14L, 30.87L, 32.70L, 34.65L, 36.71L, 38.89L, 41.20L, 43.65L, 46.25L, 49.00L, 51.91L,
    	55.00L, 58.27L, 61.74L, 65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L,
    	110.00L, 116.54L, 123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L,
    	207.65L, 220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L,
    	415.30L, 440.00L, 466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L, 698.46L, 739.99L, 783.99L,
    	830.61L, 880.00L, 932.33L, 987.77L, 1046.50L, 1108.73L, 1174.66L, 1244.51L, 1318.51L, 1396.91L, 1479.98L, 1567.98L,
    	1661.22L, 1760.00L, 1864.66L, 1975.53L, 2093.00L, 2217.46L, 2349.32L, 2489.02L, 2637.02L, 2793.83L, 2959.96L, 3135.96L,
    	3322.44L, 3520.00L, 3729.31L, 3951.07L, 4186.01L
	};

	const std::vector<long double> saxFreqs = {
    	138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 184.99L, 195.99L, 207.65L, 220.00L, 233.08L, 246.94L, 261.63L,
    	277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L,
    	554.37L, 587.33L, 622.25L, 659.26L, 698.46L, 739.99L, 783.99L, 830.61L, 880.00L
	};

	const std::vector<long double> pianoFreqs = {
    	27.50L, 29.14L, 30.87L, 32.70L, 34.65L, 36.71L, 38.89L, 41.20L, 43.65L, 46.25L, 49.00L, 51.91L,
    	55.00L, 58.27L, 61.74L, 65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L,
    	110.00L, 116.54L, 123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L,
    	220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L, 415.30L,
    	440.00L, 466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L, 698.46L, 739.99L, 783.99L, 830.61L,
    	880.00L, 932.33L, 987.77L, 1046.50L, 1108.73L, 1174.66L, 1244.51L, 1318.51L, 1396.91L, 1479.98L, 1567.98L, 1661.22L,
    	1760.00L, 1864.66L, 1975.53L, 2093.00L, 2217.46L, 2349.32L, 2489.02L, 2637.02L, 2793.83L, 2959.96L, 3135.96L, 3322.44L,
    	3520.00L, 3729.31L, 3951.07L, 4186.01L
	};
		
	const std::vector<long double> banjoFreqs = {
    	// Banjo (standard 5-string, G tuning): G2 (98.00 Hz) to G5 (783.99 Hz)
    	98.00L, 103.83L, 110.00L, 116.54L, 123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L,
    	174.61L, 185.00L, 196.00L, 207.65L, 220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L,
    	311.13L, 329.63L, 349.23L, 369.99L, 392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L,
    	554.37L, 587.33L, 622.25L, 659.25L, 698.46L, 739.99L, 783.99L
	};

	const std::vector<long double> bassFreqs = {
    	// Electric/Acoustic Bass (4-string, standard tuning): E1 (41.20 Hz) to G3 (196.00 Hz)
    	41.20L, 43.65L, 46.25L, 49.00L, 51.91L, 55.00L, 58.27L, 61.74L, 65.41L, 69.30L, 73.42L,
    	77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L, 123.47L, 130.81L,
    	138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L
	};

	const std::vector<long double> bellFreqs = {
    	// Bell (e.g., tubular bells): C4 (261.63 Hz) to C6 (1046.50 Hz)
    	261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L, 415.30L, 440.00L,
    	466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L, 698.46L, 739.99L, 783.99L,
    	830.61L, 880.00L, 932.33L, 987.77L, 1046.50L
	};

	const std::vector<long double> celloFreqs = {
    	// Cello (standard tuning): C2 (65.41 Hz) to A4 (440.00 Hz)
    	65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L,
    	123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L,
    	220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L,
    	392.00L, 415.30L, 440.00L
	};

	const std::vector<long double> clarinetFreqs = {
    	// Clarinet (B♭ clarinet): D3 (146.83 Hz) to A5 (880.00 Hz)
    	146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L, 220.00L, 233.08L, 246.94L,
    	261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L, 415.30L, 440.00L,
    	466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L, 698.46L, 739.99L, 783.99L,
    	830.61L, 880.00L
	};

	const std::vector<long double> fluteFreqs = {
    	// Flute (concert flute): C4 (261.63 Hz) to C7 (2093.00 Hz)
    	261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L, 415.30L, 440.00L,
    	466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L, 698.46L, 739.99L, 783.99L,
    	830.61L, 880.00L, 932.33L, 987.77L, 1046.50L, 1108.73L, 1174.66L, 1244.51L, 1318.51L,
    	1396.91L, 1479.98L, 1567.98L, 1661.22L, 1760.00L, 1864.66L, 1975.53L, 2093.00L
	};

	const std::vector<long double> guitarFreqs = {
    	// Guitar (6-string, standard tuning): E2 (82.41 Hz) to E5 (659.25 Hz)
    	82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L, 123.47L, 130.81L, 138.59L,
    	146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L, 220.00L, 233.08L, 246.94L,
    	261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L, 415.30L, 440.00L,
    	466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L
	};

	const std::vector<long double> marimbaFreqs = {
    	// Marimba (standard 4.3-octave): C2 (65.41 Hz) to A5 (880.00 Hz)
    	65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L,
    	123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L,
    	220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L,
    	392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L,
    	698.46L, 739.99L, 783.99L, 830.61L, 880.00L
	};

	const std::vector<long double> oboeFreqs = {
    	// Oboe: B♭3 (233.08 Hz) to A5 (880.00 Hz)
    	233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L,
    	415.30L, 440.00L, 466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L, 698.46L,
    	739.99L, 783.99L, 830.61L, 880.00L
	};

	const std::vector<long double> organFreqs = {
    	// Organ (pipe or electronic, typical range): C2 (65.41 Hz) to C6 (1046.50 Hz)
    	65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L,
    	123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L,
    	220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L,
    	392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L,
    	698.46L, 739.99L, 783.99L, 830.61L, 880.00L, 932.33L, 987.77L, 1046.50L
	};

	const std::vector<long double> sitarFreqs = {
    	// Sitar: C2 (65.41 Hz) to C5 (523.25 Hz)
    	65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L,
    	123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L,
    	220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L,
    	392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L
	};

	const std::vector<long double> steelguitarFreqs = {
    	// Steel Guitar (pedal or lap, standard tuning): C2 (65.41 Hz) to E5 (659.25 Hz)
    	65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L,
    	123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L,
    	220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L,
    	392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L
	};

	const std::vector<long double> tromboneFreqs = {
    	// Trombone (tenor): E2 (82.41 Hz) to B♭4 (466.16 Hz)
    	82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L, 123.47L, 130.81L, 138.59L,
    	146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L, 220.00L, 233.08L, 246.94L,
    	261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L, 415.30L, 440.00L,
    	466.16L
	};

	const std::vector<long double> trumpetFreqs = {
    	// Trumpet (B♭ trumpet): F♯3 (185.00 Hz) to C6 (1046.50 Hz)
    	185.00L, 196.00L, 207.65L, 220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L,
    	329.63L, 349.23L, 369.99L, 392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L, 554.37L,
    	587.33L, 622.25L, 659.25L, 698.46L, 739.99L, 783.99L, 830.61L, 880.00L, 932.33L, 987.77L,
    	1046.50L
	};

	const std::vector<long double> tubaFreqs = {
    	// Tuba (B♭ tuba): D1 (36.71 Hz) to F3 (174.61 Hz)
    	36.71L, 38.89L, 41.20L, 43.65L, 46.25L, 49.00L, 51.91L, 55.00L, 58.27L, 61.74L, 65.41L,
    	69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L, 123.47L,
    	130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L
	};

	const std::vector<long double> violinFreqs = {
    	// Violin: G3 (196.00 Hz) to A6 (1760.00 Hz)
    	196.00L, 207.65L, 220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L,
    	349.23L, 369.99L, 392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L, 554.37L, 587.33L,
    	622.25L, 659.25L, 698.46L, 739.99L, 783.99L, 830.61L, 880.00L, 932.33L, 987.77L, 1046.50L,
    	1108.73L, 1174.66L, 1244.51L, 1318.51L, 1396.91L, 1479.98L, 1567.98L, 1661.22L, 1760.00L
	};

	const std::vector<long double> xylophoneFreqs = {
    	// Xylophone (standard 3.5-octave): F3 (174.61 Hz) to C7 (2093.00 Hz)
    	174.61L, 185.00L, 196.00L, 207.65L, 220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L,
    	311.13L, 329.63L, 349.23L, 369.99L, 392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L,
    	554.37L, 587.33L, 622.25L, 659.25L, 698.46L, 739.99L, 783.99L, 830.61L, 880.00L, 932.33L,
    	987.77L, 1046.50L, 1108.73L, 1174.66L, 1244.51L, 1318.51L, 1396.91L, 1479.98L, 1567.98L,
    	1661.22L, 1760.00L, 1864.66L, 1975.53L, 2093.00L
	};

	const std::vector<long double> syntharpFreqs = {
    	// Syntharp (assuming broad synth range): C2 (65.41 Hz) to C6 (1046.50 Hz)
    	65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L,
    	123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L,
    	220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L,
    	392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L,
    	698.46L, 739.99L, 783.99L, 830.61L, 880.00L, 932.33L, 987.77L, 1046.50L
	};

	const std::vector<long double> leadsynthFreqs = {
    	// Leadsynth (assuming melodic synth range): C3 (130.81 Hz) to C6 (1046.50 Hz)
    	130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L, 220.00L,
    	233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L,
    	415.30L, 440.00L, 466.16L, 493.88L, 523.25L, 554.37L, 587.33L, 622.25L, 659.25L, 698.46L,
    	739.99L, 783.99L, 830.61L, 880.00L, 932.33L, 987.77L, 1046.50L
	};

	const std::vector<long double> padFreqs = {
    	// Pad (assuming ambient synth range): C2 (65.41 Hz) to C5 (523.25 Hz)
    	65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L, 103.83L, 110.00L, 116.54L,
    	123.47L, 130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L,
    	220.00L, 233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L,
    	392.00L, 415.30L, 440.00L, 466.16L, 493.88L, 523.25L
	};

	const std::vector<long double> subbassFreqs = {
    	// Subbass (low-frequency synth): C1 (32.70 Hz) to G2 (98.00 Hz)
    	32.70L, 34.65L, 36.71L, 38.89L, 41.20L, 43.65L, 46.25L, 49.00L, 51.91L, 55.00L, 58.27L,
    	61.74L, 65.41L, 69.30L, 73.42L, 77.78L, 82.41L, 87.31L, 92.50L, 98.00L
	};

	const std::vector<long double> vocalFreqs = {
    	// Vocal (assuming typical human vocal range, mixed voices): C3 (130.81 Hz) to C5 (523.25 Hz)
    	130.81L, 138.59L, 146.83L, 155.56L, 164.81L, 174.61L, 185.00L, 196.00L, 207.65L, 220.00L,
    	233.08L, 246.94L, 261.63L, 277.18L, 293.66L, 311.13L, 329.63L, 349.23L, 369.99L, 392.00L,
    	415.30L, 440.00L, 466.16L, 493.88L, 523.25L
	};

	// Unpitched/Percussive Instruments: Limited or no specific frequencies
	const std::vector<long double> kickFreqs = { 60.00L };
	const std::vector<long double> snareFreqs = { 200.00L };
	const std::vector<long double> cymbalFreqs = { 400.00L };
	const std::vector<long double> hihatFreqs = { 450.00L };
	const std::vector<long double> clapFreqs = { 300.00L };
	const std::vector<long double> tambourineFreqs = { 350.00L };
	const std::vector<long double> tomFreqs = { 80.00L, 100.00L, 120.00L, 150.00L, 200.00L, 250.00L };

    std::vector<Note> melodyMotif;
    std::map<std::string, Part> sectionTemplates;
    std::map<std::string, std::vector<int>> chordProgressions;

    long double getClosestFreq(long double target, const std::vector<long double>& freqPool) {
        if (!std::isfinite(target) || target <= 0.0L) {
            ::SDL_Log("Invalid frequency target %.2Lf, returning %.2Lf Hz", target, freqPool[0]);
            return freqPool[0];
        }
        long double closest = freqPool[0];
        long double minDiff = std::abs(target - closest);
        for (long double freq : freqPool) {
            long double diff = std::abs(target - freq);
            if (diff < minDiff) {
                minDiff = diff;
                closest = freq;
            }
        }
        return closest;
    }

    // Overload for default frequency pool
    long double getClosestFreq(long double target) {
        return getClosestFreq(target, availableFreqs);
    }

    long double snapToBeatGrid(long double time, long double bpm) {
        long double sixteenthNote = 60.0L / (bpm * 4);
        return std::round(time / sixteenthNote) * sixteenthNote;
    }

std::string generateTitle() {
    	::SDL_Log("Generating song title");
		static thread_local AudioUtils::RandomGenerator rng;
		// todo: genre affecting the names so they are not random.

    	// Word lists
    	std::vector<std::string> adjectives = {
        	"Ancient", "Astral", "Auroral", "Blazing", "Bleak", "Bold", "Breezy", "Brilliant", "Burning", "Celestial",
        	"Charmed", "Chilling", "Cosmic", "Crimson", "Crystal", "Dancing", "Dazzling", "Deep", "Desolate", "Divine",
        	"Echoing", "Electric", "Elusive", "Emerald", "Enchanted", "Endless", "Ethereal", "Fading", "Feral", "Fierce",
        	"Flickering", "Floating", "Forbidden", "Frantic", "Frosty", "Furious", "Gilded", "Glimmering", "Glowing", "Golden",
        	"Harmonic", "Haunted", "Hazy", "Infinite", "Iridescent", "Jagged", "Jubilant", "Livid", "Lone", "Lucid",
        	"Luminous", "Lunar", "Lush", "Magnetic", "Majestic", "Mellow", "Midnight", "Misty", "Mystic", "Neon",
        	"Obsidian", "Opulent", "Pale", "Phantom", "Platinum", "Pristine", "Pulsing", "Radiant", "Raging", "Resonant",
        	"Restless", "Reverent", "Rhythmic", "Rippling", "Sable", "Sacred", "Sapphire", "Savage", "Scarlet", "Seething",
        	"Serene", "Shadowy", "Shimmering", "Silent", "Silver", "Sizzling", "Smoky", "Solar", "Solemn", "Sonic",
        	"Spectral", "Spiraling", "Stellar", "Stormy", "Sublime", "Sultry", "Swift", "Tempestuous", "Tender", "Thundering",
        	"Timeless", "Torn", "Tranquil", "Twilight", "Vast", "Velvet", "Vibrant", "Vivid", "Wailing", "Wandering",
        	"Whispering", "Wild", "Wistful", "Withered", "Writhing", "Zephyr", "Blissful", "Chaotic", "Dreary", "Eclipsed",
        	"Exalted", "Frenzied", "Gleaming", "Hollow", "Illusive", "Jazzy", "Nebulous", "Noir", "Primal", "Quivering",
        	"Rustic", "Shattered", "Soaring", "Spiky", "Tangled", "Turbulent", "Unraveled", "Vortex", "Warming", "Zealous"
    	};
    	std::vector<std::string> nouns = {
        	"Abyss", "Aether", "Alley", "Anchor", "Aria", "Aurora", "Banner", "Beacon", "Blaze", "Bloom",
        	"Boulder", "Breeze", "Bridge", "Brook", "Canyon", "Cascade", "Cavern", "Chasm", "Chord", "Cliff",
        	"Cloud", "Comet", "Cove", "Crest", "Crush", "Current", "Dawn", "Delta", "Desert", "Drift",
        	"Dune", "Dusk", "Echo", "Eclipse", "Ember", "Empire", "Essence", "Fable", "Falls", "Field",
        	"Flame", "Flood", "Flow", "Fog", "Forest", "Fountain", "Frost", "Galaxy", "Glade", "Glow",
        	"Gorge", "Grove", "Halo", "Harbor", "Haven", "Haze", "Heart", "Heath", "Horizon", "Hymn",
        	"Isle", "Journey", "Jungle", "Lagoon", "Lantern", "Ledge", "Light", "Luster", "Meadow", "Mirage",
        	"Mist", "Moon", "Moor", "Mountain", "Nebula", "Night", "Oasis", "Ocean", "Orbit", "Peak",
        	"Plain", "Pulse", "Quest", "Rain", "Ravine", "Ray", "Reef", "Rift", "Ripple", "River",
        	"Ruin", "Sands", "Sea", "Shade", "Shadow", "Shore", "Sky", "Snow", "Spark", "Sphere",
        	"Spire", "Spring", "Star", "Stone", "Storm", "Stream", "Summit", "Sun", "Surge", "Swamp",
        	"Symphony", "Tide", "Trail", "Tundra", "Vale", "Valley", "Vapor", "Veil", "Vine", "Vista",
        	"Void", "Wave", "Whirl", "Wind", "Wood", "Wraith", "Pickle", "Crimson", "Dawn", "Dwell",
        "Flicker", "Glimpse", "Hush", "Murmur", "Ridge", "Rush", "Sail", "Shine", "Twilight", "Vortex"
    	};
    	std::vector<std::string> verbs = {
        	"Blaze", "Bloom", "Break", "Breathe", "Burn", "Burst", "Carve", "Chase", "Clash", "Climb",
        	"Crash", "Crawl", "Dance", "Dash", "Dive", "Drift", "Drown", "Echo", "Fade", "Fall",
        	"Flicker", "Float", "Flow", "Fly", "Forge", "Freeze", "Gallop", "Gleam", "Glide", "Glow",
        	"Grow", "Halt", "Howl", "Ignite", "Leap", "Linger", "Lurch", "Melt", "Mend", "Merge",
        	"Murmur", "Plunge", "Pulse", "Race", "Rage", "Reach", "Rip", "Rise", "Roar", "Rush",
        	"Sail", "Scorch", "Scream", "Seethe", "Shatter", "Shine", "Sing", "Sink", "Soar", "Spin",
        	"Sprint", "Stir", "Surge", "Sway", "Sweep", "Swirl", "Tear", "Thrive", "Twist", "Vanish",
        	"Wander", "Wave", "Weave", "Whirl", "Whisper", "Wield", "Wilt", "Writhe", "Yearn", "Yield"
    	};
    	std::vector<std::string> adverbs = {
        	"Ardently", "Blissfully", "Boldly", "Brightly", "Calmly", "Carefully", "Cautiously", "Cheerfully", "Clearly", "Closely",
        	"Darkly", "Deeply", "Delicately", "Eagerly", "Easily", "Faintly", "Fiercely", "Freely", "Gently", "Gleefully",
        	"Gracefully", "Happily", "Harshly", "Heavily", "Highly", "Humbly", "Keenly", "Lightly", "Loudly", "Madly",
        	"Meekly", "Merrily", "Proudly", "Quickly", "Quietly", "Rapidly", "Sadly", "Sharply", "Silently", "Slowly",
        	"Softly", "Solemnly", "Steadily", "Strongly", "Swiftly", "Tenderly", "Truly", "Vividly", "Warmly", "Wildly"
    	};
    	std::vector<std::string> prepositions = {
        	"Above", "Across", "Against", "Along", "Amid", "Among", "Around", "At", "Before", "Behind",
        	"Beneath", "Beside", "Between", "Beyond", "Into", "Over", "Through", "Toward", "Under", "Within"
    	};
		
    	// Title templates using long double to influence title generation
    	std::vector<std::function<std::string(long double)>> titleTemplates = {
        	[&](long double value) { // Adj Noun, with value-based suffix
            	std::string base = adjectives[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(adjectives.size() - 1)))] + " " +
                	               nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))];
            	return value > 0.5L ? base + " Echo" : base; // Example: use value to add suffix
        	},
        	[&](long double value) { // Verb the Noun, choose noun list based on value
            	auto& noun_list = value > 0.5L ? nouns : adjectives; // Use adjectives as nouns if value > 0.5
            	return verbs[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(verbs.size() - 1)))] + " the " +
            	       noun_list[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(noun_list.size() - 1)))];
        	},
        	[&](long double value) { // Adj Noun Prep Noun
            	return adjectives[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(adjectives.size() - 1)))] + " " +
            			nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))] + " " +
            			prepositions[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(prepositions.size() - 1)))] + " " +
            			nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))];
        	},
        	[&](long double value) { // Verb Adv
            	return verbs[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(verbs.size() - 1)))] + " " +
            	       adverbs[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(adverbs.size() - 1)))];
        	},
        	[&](long double value) { // Adj Noun Prep Adj Noun
            	return adjectives[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(adjectives.size() - 1)))] + " " +
                		nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))] + " " +
                		prepositions[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(prepositions.size() - 1)))] + " " +
                		adjectives[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(adjectives.size() - 1)))] + " " +
            			nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))];
        	},
        	[&](long double value) { // Noun Prep Noun
            	return nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))] + " " +
                	   prepositions[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(prepositions.size() - 1)))] + " " +
            	       nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))];
        	},
        	[&](long double value) { // Verb the Adj Noun
            	return verbs[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(verbs.size() - 1)))] + " the " +
                	   adjectives[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(adjectives.size() - 1)))] + " " +
            	       nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))];
        	},
        	[&](long double value) { // Adj Verb Noun
            	return adjectives[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(adjectives.size() - 1)))] + " " +
                	   verbs[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(verbs.size() - 1)))] + " " +
            	       nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))];
        	},
        	[&](long double value) { // Adv Verb Noun
            	return adverbs[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(adverbs.size() - 1)))] + " " +
                	   verbs[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(verbs.size() - 1)))] + " " +
            	       nouns[static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(nouns.size() - 1)))];
        	}
    	};

    	// Generate title
    	size_t index = static_cast<size_t>(rng.dist(0.0L, static_cast<long double>(titleTemplates.size() - 1)));
    	std::string title = titleTemplates[index](rng.dist(0.0L, 1.0L));

    	return title;
	}

	long double getRandomDuration(Genre genre, long double sectionProgress, long double bpm) {
		static thread_local AudioUtils::RandomGenerator rng;
    	// Validate BPM
    	if (!std::isfinite(bpm) || bpm <= 0) {
        	::SDL_Log("Invalid BPM %.2Lf, using default 80 BPM", bpm);
        	bpm = 80.0L;
    	}

    	// Get duration weights for the genre
    	const auto& weights = genreDurationWeights.at(genre);
    	std::vector<long double> adjustedWeights = weights;

    	// Adjust weights for middle of section (if applicable)
    	if (sectionProgress > 0.4L && sectionProgress < 0.6L) {
        	if (adjustedWeights.size() > 10) {
            	adjustedWeights[10] += 0.05L; // Bias towards longer durations
        	}
        	if (adjustedWeights.size() > 12) {
            	adjustedWeights[12] += 0.05L; // Bias towards even longer durations
        	}
    	}

    	// Ensure weights and durations are compatible
    	if (adjustedWeights.size() != durations.size()) {
        	::SDL_Log("Error: Mismatch between weights (%zu) and durations (%zu) for genre %d",
                  	adjustedWeights.size(), durations.size(), static_cast<int>(genre));
        	return 60.0L / (bpm * 16); // Fallback to minimum duration
    	}

    	// Generate random duration index using discrete distribution
    	size_t durationIndex = rng.dist(adjustedWeights.begin(), adjustedWeights.end());
   		long double duration = durations[durationIndex];

    	// Ensure duration is at least a 16th note
    	long double minDur = 60.0L / (bpm * 16);
    	return static_cast<long double>(std::max(minDur, duration));
	}

std::vector<std::vector<int>> getChordProgressions(const std::string& scaleName, Genre g) {
    // Define base progressions for each scale
    std::vector<std::vector<int>> baseProgs;
    if (scaleName == "major") {
        baseProgs = {{1,4,5,1},{1,5,6,4},{1,6,4,5},{1,2,5,4},{1,3,6,4},{2,5,1,4},{1,4,6,5},{1,5,4,6},{4,5,1,6},{1,2,4,5},{6,4,1,5},{1,3,4,5},{2,5,6,4},{1,4,2,5},{1,6,2,5}};
    } else if (scaleName == "minor") {
        baseProgs = {{6,4,1,5},{6,3,4,5},{6,7,1,4},{6,2,5,3},{6,1,4,7},{3,6,4,5},{6,4,7,1},{6,5,3,4},{4,6,7,1},{6,2,4,5},{1,6,4,5},{6,3,7,4},{2,5,6,1},{6,4,2,5}};
    } else if (scaleName == "dorian") {
        baseProgs = {{2,7,1,4},{2,5,6,7},{2,4,7,1},{2,1,4,5},{2,6,4,7},{4,2,7,1},{2,5,4,6},{2,7,4,1},{1,2,5,6},{2,4,1,7}};
    } else if (scaleName == "mixolydian") {
        baseProgs = {{5,1,4,7},{5,6,1,4},{5,3,6,7},{5,4,1,6},{1,5,4,6},{5,7,1,4},{4,5,6,1},{5,1,6,4},{5,4,7,1},{6,5,1,4}};
    } else if (scaleName == "blues") {
        baseProgs = {{1,4,1,5},{1,5,4,1},{1,4,5,1},{1,4,1,4},{4,1,5,4},{1,5,1,4},{4,5,1,1},{1,1,4,5},{5,4,1,1},{1,4,5,5}};
    } else if (scaleName == "harmonic_minor") {
        baseProgs = {{1,6,3,5},{1,4,6,7},{1,5,6,3},{1,7,3,6},{6,1,4,5},{1,3,7,6},{4,1,6,7},{1,6,5,3},{7,1,4,6},{1,4,7,3}};
    } else if (scaleName == "whole_tone") {
        baseProgs = {{1,3,5,1},{1,4,2,5},{1,5,3,4},{2,1,4,5},{1,2,5,3},{3,1,4,2}};
    } else if (scaleName == "pentatonic_major") {
        baseProgs = {{1,4,5,1},{1,5,6,4},{1,6,4,5},{1,2,5,4},{4,1,6,5},{1,4,2,5}};
    } else if (scaleName == "pentatonic_minor") {
        baseProgs = {{6,4,1,5},{6,1,4,5},{4,6,5,1},{6,5,4,1},{1,6,4,5},{6,4,5,1}};
    } else {
        baseProgs = {{1,4,5,4}}; // Default fallback
    }

    // Genre-specific progressions
    std::vector<std::vector<int>> genreProgs;
    switch (g) {
        case JAZZ: case BLUES:
            genreProgs = {{2,5,1,6},{2,5,1,4},{2,7,3,6},{1,6,2,5},{2,5,3,6},{1,4,2,5},{2,5,6,1},{3,6,2,5},{1,5,2,5}};
            break;
        case CLASSICAL:
            genreProgs = {{1,6,2,5},{1,4,6,5},{4,1,5,6},{1,3,4,5},{1,6,4,2},{2,5,1,6},{1,7,4,5},{1,3,6,2}};
            break;
        case POP: case ROCK: case COUNTRY:
            genreProgs = {{1,5,4,6},{4,5,1,6},{1,4,6,2},{1,6,5,4},{2,5,4,1},{1,2,6,5},{4,1,6,5},{1,5,6,2},{6,4,5,1}};
            break;
        case GOSPEL: case SOUL:
            genreProgs = {{1,4,6,5},{1,6,4,5},{4,1,5,6},{1,2,5,4},{6,5,1,4},{1,3,6,5},{2,5,6,1},{1,4,2,5}};
            break;
        case METAL:
            genreProgs = {{1,7,4,5},{1,4,7,1},{6,7,1,4},{1,5,4,7},{1,3,7,4},{7,1,4,6},{1,6,7,4},{4,7,1,5}};
            break;
        case LATIN:
            genreProgs = {{1,4,2,5},{1,6,4,5},{4,1,5,2},{2,5,1,4},{1,4,6,2},{6,4,1,5},{1,2,4,6},{4,5,2,1}};
            break;
        case EDM: case TECHNO:
            genreProgs = {{1,4,5,6},{4,5,1,6},{1,6,4,5},{6,4,1,5},{1,5,4,6},{4,1,6,5},{1,4,2,5},{2,5,1,4},{1,6,5,4},{4,6,1,5},{1,5,6,2},{6,5,4,1},{1,4,6,2},{2,6,4,1},{1,2,5,6},{4,5,6,1}};
            break;
        case REGGAE:
            genreProgs = {{1,4,5,1},{1,6,4,5},{4,1,6,5},{1,5,6,4},{2,5,1,4},{6,4,1,5},{1,4,2,5},{1,6,5,4},{4,5,1,6},{1,2,6,5},{6,5,4,1},{1,4,5,6},{4,6,1,5},{1,5,4,2}};
            break;
        case AMBIENT:
            genreProgs = {{1,3,5,4},{1,6,4,5},{4,1,5,6},{1,4,6,3},{6,4,1,5},{1,5,3,4},{2,6,4,1},{1,4,5,2},{1,6,5,4},{4,5,1,6},{1,3,4,6},{6,5,4,1},{1,4,2,6},{2,5,1,4},{1,6,3,5}};
            break;
        case HIPHOP: case RAP:
            genreProgs = {{6,4,1,5},{1,6,4,5},{4,1,6,5},{1,5,6,4},{6,5,4,1},{1,4,2,5},{2,5,1,4},{6,4,5,1},{1,6,5,4},{4,6,1,5},{1,2,6,5},{6,5,1,4},{1,4,5,6},{4,5,6,1}};
            break;
        case FOLK:
            genreProgs = {{1,4,5,1},{1,6,4,5},{4,1,5,6},{1,2,5,4},{1,3,4,5},{6,4,1,5},{1,4,2,5},{2,5,1,4}};
            break;
        case FUNK:
            genreProgs = {{1,5,4,1},{6,4,1,5},{1,7,4,5},{1,4,6,7},{2,5,1,4},{1,6,5,4},{4,1,6,5},{1,4,2,5}};
            break;
        case WORLD:
            genreProgs = {{1,4,6,5},{2,7,1,4},{6,4,1,5},{1,3,4,6},{4,1,5,2},{1,6,2,5},{2,5,6,1},{1,4,7,3}};
            break;
        default:
            genreProgs = {{1,4,5,1},{1,5,6,4},{1,6,4,5},{4,5,1,6},{1,4,2,5},{2,5,1,4},{6,4,1,5},{1,4,5,6}};
            break;
    }

    // Combine and deduplicate progressions
    std::set<std::vector<int>> uniqueProgs(baseProgs.begin(), baseProgs.end());
    uniqueProgs.insert(genreProgs.begin(), genreProgs.end());

    return {uniqueProgs.begin(), uniqueProgs.end()};
}

	std::vector<long double> buildChord(long double degree, const std::string& scaleName, long double rootFreq, Genre g, long double inversion = 0) {
    	static thread_local AudioUtils::RandomGenerator rng;
    	if (!std::isfinite(rootFreq) || rootFreq <= 0.0L) {
        	::SDL_Log("Invalid rootFreq %.2Lf in buildChord, using 440.0 Hz", rootFreq);
        	rootFreq = 440.0L;
    	}
    	const auto& intervals = scales.at(scaleName);
    	rootFreq = getClosestFreq(rootFreq);
    	std::vector<long double> chord;
    	long double baseIdx = std::fmod(degree - 1.0L + static_cast<long double>(intervals.size()), static_cast<long double>(intervals.size()));

    	// Define chord intervals based on genre
    	std::vector<long double> chordIntervals;
    	long double randVal = rng.random_L(); // [0,1]

    	if (g == JAZZ || g == BLUES || g == GOSPEL || g == SOUL || g == CLASSICAL_JAZZ_FUSION) {
        	chordIntervals = (randVal < 0.5L) ? std::vector<long double>{0.0L, 4.0L, 7.0L, 11.0L} 
        	                                : std::vector<long double>{0.0L, 4.0L, 7.0L, 10.0L};
    	} else if (g == METAL || g == PUNK) {
        	chordIntervals = (degree == 1) ? std::vector<long double>{0.0L, 7.0L} 
        	                              : std::vector<long double>{0.0L, 4.0L, 7.0L};
    	} else if (g == POP || g == ROCK || g == COUNTRY || g == INDIE || g == FOLK || g == BLUEGRASS) {
        	chordIntervals = (randVal < 0.3333L) ? std::vector<long double>{0.0L, 4.0L, 7.0L, 10.0L} 
        	                                    : std::vector<long double>{0.0L, 4.0L, 7.0L};
    	} else if (g == EDM || g == TECHNO || g == DUBSTEP || g == ELECTRONICA || g == HIPHOP || g == RAP || g == TRAP) {
        	if (randVal < 0.25L) {
            	chordIntervals = {0.0L, 2.0L, 7.0L};
        	} else if (randVal < 0.5L) {
            	chordIntervals = {0.0L, 5.0L, 7.0L};
        	} else {
            	chordIntervals = {0.0L, 4.0L, 7.0L};
        	}
    	} else if (g == AMBIENT || g == CLASSICAL || g == NEW_AGE) {
        	chordIntervals = (randVal < 0.3333L) ? std::vector<long double>{0.0L, 4.0L, 7.0L, 14.0L} 
                                            	: std::vector<long double>{0.0L, 4.0L, 7.0L};
    	} else if (g == LATIN || g == REGGAE || g == REGGAETON) {
        	chordIntervals = (randVal < 0.3333L) ? std::vector<long double>{0.0L, 4.0L, 7.0L, 10.0L} 
        	                                    : std::vector<long double>{0.0L, 4.0L, 7.0L};
    	} else if (g == RNB || g == DISCO || g == FUNK) {
        	chordIntervals = (randVal < 0.5L) ? std::vector<long double>{0.0L, 4.0L, 7.0L, 10.0L} 
        	                                 : std::vector<long double>{0.0L, 3.0L, 7.0L, 10.0L};
    	} else if (g == WORLD) {
        	chordIntervals = (randVal < 0.5L) ? std::vector<long double>{0.0L, 4.0L, 7.0L, 9.0L} 
        	                                 : std::vector<long double>{0.0L, 4.0L, 7.0L};
    	} else {
        	chordIntervals = {0.0L, 4.0L, 7.0L}; // Default to major triad
    	}

    	// Build chord frequencies
    	for (long double offset : chordIntervals) {
        	long double noteIdx = std::fmod(baseIdx + offset, static_cast<long double>(intervals.size()));
        	long double freq = rootFreq * std::pow(2.0L, intervals[static_cast<size_t>(noteIdx)] / 12.0L);
        	chord.push_back(getClosestFreq(freq));
    	}

    	// Apply inversions
    	if (inversion > 0 && !chord.empty()) {
        	for (int i = 0; i < inversion; ++i) {
            	long double nextFreq = chord[0] * 2.0L;
            	chord.erase(chord.begin());
            	if (nextFreq > availableFreqs.back()) nextFreq = availableFreqs.back();
            	chord.push_back(getClosestFreq(nextFreq));
        	}
    	}

    	return chord;
	}

	std::vector<Note> generateMotif(Genre g, const std::string& scaleName, long double rootFreq, long double bpm) {
		static thread_local AudioUtils::RandomGenerator rng;
    	std::vector<Note> motif;
    	const auto& intervals = scales.at(scaleName);
    	long double t = 0.0L;
    	long double motifDur = 60.0L / bpm; // Duration of one beat

    	// Set number of notes based on genre
    	int numNotes;
    	switch (g) {
        	case JAZZ: // Jazz
        	case BLUES: // and Blues
        	case CLASSICAL_JAZZ_FUSION: // and
        	case LATIN:
        	case REGGAE:
        	case REGGAETON: // if any of the ands or Reggaeton
            	numNotes = 3; // Shorter, syncopated motifs
            	break;
        	case EDM:
        	case TECHNO:
        	case DUBSTEP:
        	case TRAP:
        	case ELECTRONICA:
            	numNotes = 5; // Repetitive, rhythmic motifs
            	break;
        	case POP:
        	case ROCK:
        	case COUNTRY:
        	case INDIE:
        	case FOLK:
        	case BLUEGRASS:
            	numNotes = 4; // Standard melodic motifs
            	break;
        	case AMBIENT:
        	case CLASSICAL:
        	case NEW_AGE:
    	        numNotes = 2; // Sparse, sustained motifs
	            break;
        	case RNB:
        	case DISCO:
        	case FUNK:
        	case SOUL:
            	numNotes = 3; // Groovy, rhythmic motifs
            	break;
        	case HIPHOP:
        	case RAP:
            	numNotes = 4; // Rhythmic, punchy motifs
            	break;
        	case METAL:
        	case PUNK:
            	numNotes = 3; // Aggressive, short motifs
            	break;
        	case WORLD:
            	numNotes = 4; // Varied, culturally inspired motifs
            	break;
        	default:
        	    numNotes = 4; // Default to standard motif length
    	}

    	// Initial note index
    	size_t currentIdx = static_cast<size_t>(rng.random_L() * intervals.size());
    	long double currentFreq = getClosestFreq(rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L));

    	for (int i = 0; i < numNotes && t < motifDur; ++i) {
        	Note note;
        	note.startTime = t;
        	// Adjust duration scaling based on genre
        	long double durationScale = (g == JAZZ || g == BLUES || g == LATIN || g == REGGAE || g == REGGAETON || 
            	                        g == RNB || g == FUNK || g == SOUL) ? 0.75L : 0.5L;
        	note.duration = getRandomDuration(g, durationScale, bpm);
        	note.freq = currentFreq;
        	note.volume = 0.5L;
        	note.velocity = 0.8L + 0.2L * rng.random_L(); // Velocity in [0.8, 1.0]
        	motif.push_back(note);
        	t += note.duration;

        	// Random step: up, down, or stay
        	long double randVal = rng.random_L();
        	int step = (randVal < 0.3333L) ? -1 : (randVal < 0.6666L) ? 1 : 0;
        	currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
        	currentFreq = getClosestFreq(rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L));
    	}

    	return motif;
	}

	Part varyPart(const Part& original, long double timeOffset, Genre g, long double intensity = 1.0L, bool transpose = false, long double transposeSemitones = 0.0L, const std::string& scaleName = "") {
    	static thread_local AudioUtils::RandomGenerator rng;
    	Part varied = original;
    	varied.notes.clear();
    	varied.panAutomation.clear();
    	varied.volumeAutomation.clear();
    	varied.reverbMixAutomation.clear();

    	// Validate inputs
    	if (intensity < 0.0L) {
        	::SDL_Log("Invalid intensity %.2Lf in varyPart, using 1.0", intensity);
        	intensity = 1.0L;
    	}

    	// Scale-constrained transposition (optional)
    	const auto& intervals = scaleName.empty() ? std::vector<long double>{} : scales.at(scaleName);
    	std::vector<long double> scaleFreqs;
    	if (transpose && !scaleName.empty() && !intervals.empty()) {
        	for (long double interval : intervals) {
            	scaleFreqs.push_back(getClosestFreq(original.notes.empty() ? 440.0L : original.notes[0].freq * std::pow(2.0L, interval / 12.0L)));
        	}
    	}

    	for (const auto& note : original.notes) {
        	Note newNote = note;
        	newNote.startTime += timeOffset;
        	if (newNote.startTime < 0.0L) newNote.startTime = 0.0L;

        	// Apply intensity with genre-specific scaling
        	long double intensityScale = intensity;
        	if (g == METAL || g == PUNK || g == DUBSTEP || g == TRAP) {
            	intensityScale *= (1.0L + 0.2L * rng.random_L()); // Boost intensity for aggressive genres
        	} else if (g == AMBIENT || g == NEW_AGE || g == CLASSICAL) {
            	intensityScale *= (0.8L + 0.2L * rng.random_L()); // Softer dynamics for ambient genres
        	}
        	newNote.volume = std::clamp(newNote.volume * intensityScale, 0.0L, 1.0L);
        	newNote.velocity = std::clamp(newNote.velocity * intensityScale, 0.0L, 1.0L);

        	// Transpose if requested
        	if (transpose) {
            	if (!scaleFreqs.empty()) {
                	// Find closest frequency in scale
                	long double transposedFreq = note.freq * std::pow(2.0L, transposeSemitones / 12.0L);
                	long double closestFreq = scaleFreqs[0];
                	long double minDiff = std::abs(transposedFreq - closestFreq);
                	for (long double freq : scaleFreqs) {
                    	long double diff = std::abs(transposedFreq - freq);
                    	if (diff < minDiff) {
                	        minDiff = diff;
            	            closestFreq = freq;
        	            }
    	            }
	                newNote.freq = getClosestFreq(closestFreq);
            	} else {
                	newNote.freq = getClosestFreq(newNote.freq * std::pow(2.0L, transposeSemitones / 12.0L));
            	}
        	}

        	// Apply duration variation based on genre
        	long double durationModProb, durationModRange;
        	switch (g) {
            	case JAZZ:
            	case BLUES:
            	case LATIN:
            	case REGGAE:
            	case REGGAETON:
            	case RNB:
            	case FUNK:
            	case SOUL:
        	        durationModProb = 0.5L; // More frequent variation for syncopated genres
    	            durationModRange = 0.3L; // Wider range for rhythmic flexibility
	                break;
            	case EDM:
            	case TECHNO:
            	case DUBSTEP:
            	case TRAP:
            	case ELECTRONICA:
                	durationModProb = 0.2L; // Less variation for consistent rhythms
                	durationModRange = 0.1L; // Narrower range for precision
                	break;
            	case AMBIENT:
            	case CLASSICAL:
            	case NEW_AGE:
                	durationModProb = 0.3L; // Moderate variation for sustained notes
                	durationModRange = 0.4L; // Wider range for expressive timing
                	break;
            	case METAL:
            	case PUNK:
                	durationModProb = 0.3L; // Moderate variation for aggressive rhythms
                	durationModRange = 0.2L; // Moderate range for punchiness
                	break;
            	case POP:
            	case ROCK:
            	case COUNTRY:
            	case INDIE:
            	case FOLK:
            	case BLUEGRASS:
            	case HIPHOP:
            	case RAP:
                	durationModProb = 0.3L; // Standard variation
                	durationModRange = 0.2L; // Standard range
                	break;
            	case WORLD:
                	durationModProb = 0.4L; // Slightly more variation for cultural diversity
                	durationModRange = 0.3L; // Wider range for unique rhythms
                	break;
            	default:
                	durationModProb = 0.3333L; // Default to original probability
            	    durationModRange = 0.2L; // Default to original range
        	}
        	if (rng.random_L() < durationModProb) {
            	newNote.duration *= (1.0L - durationModRange + 2.0L * durationModRange * rng.random_L());
        	}
        	if (newNote.duration < 0.01L) newNote.duration = 0.01L; // Prevent zero/negative durations

        	varied.notes.push_back(newNote);
    	}

    	// Apply automation variations
    	for (const auto& [time, value] : original.panAutomation) {
        	long double newValue = value;
        	if (g == AMBIENT || g == NEW_AGE || g == CLASSICAL) {
            	newValue += 0.2L * (rng.random_L() - 0.5L); // Subtle panning variation
            	newValue = std::clamp(newValue, -1.0L, 1.0L);
        	}
        	varied.panAutomation.emplace_back(time + timeOffset, newValue);
    	}
    	for (const auto& [time, value] : original.volumeAutomation) {
    		long double newValue = value * intensity;
    		if (g == JAZZ || g == BLUES || g == RNB || g == FUNK || g == SOUL) {
        		newValue *= (0.9L + 0.2L * rng.random_L()); // Dynamic variation
        		newValue = std::clamp(newValue, 0.0L, 1.0L);
    		}
    		varied.volumeAutomation.emplace_back(time + timeOffset, newValue);
		}
    	for (const auto& [time, value] : original.reverbMixAutomation) {
        	long double newValue = value;
        	if (g == AMBIENT || g == NEW_AGE || g == CLASSICAL) {
            	newValue += 0.1L * (rng.random_L() - 0.5L); // Subtle reverb variation
            	newValue = std::clamp(newValue, 0.0L, 1.0L);
        	}
        	varied.reverbMixAutomation.emplace_back(time + timeOffset, newValue);
    	}

    	return varied;
	}

Part generateMelody(Genre g, const std::string& scaleName, long double rootFreq, long double totalDur, const std::vector<Section>& sections, long double bpm) {
    static thread_local AudioUtils::RandomGenerator rng;
    Part melody;
    std::map<Genre, std::vector<std::string>> genreInstruments = {
        {ROCK, {"guitar", "piano", "organ", "leadsynth", "violin"}},
        {METAL, {"guitar", "leadsynth", "bass"}},
        {PUNK, {"guitar", "bass", "organ"}},
        {JAZZ, {"saxophone", "trumpet", "piano", "clarinet", "trombone", "vocal"}},
        {BLUES, {"guitar", "saxophone", "piano", "organ", "vocal"}},
        {CLASSICAL, {"violin", "cello", "piano", "flute", "oboe", "clarinet", "trumpet", "trombone", "tuba"}},
        {CLASSICAL_JAZZ_FUSION, {"saxophone", "piano", "violin", "trumpet", "guitar"}},
        {POP, {"piano", "guitar", "syntharp", "leadsynth", "vocal"}},
        {COUNTRY, {"guitar", "steelguitar", "banjo", "violin", "piano", "vocal"}},
        {BLUEGRASS, {"banjo", "violin", "guitar", "steelguitar", "vocal"}},
        {FOLK, {"guitar", "banjo", "violin", "vocal"}},
        {EDM, {"leadsynth", "syntharp", "pad", "subbass", "piano"}},
        {TECHNO, {"leadsynth", "syntharp", "pad", "subbass"}},
        {DUBSTEP, {"leadsynth", "subbass", "pad", "syntharp"}},
        {ELECTRONICA, {"leadsynth", "syntharp", "pad", "subbass"}},
        {HIPHOP, {"piano", "leadsynth", "vocal", "syntharp", "subbass"}},
        {RAP, {"piano", "leadsynth", "vocal", "syntharp", "subbass"}},
        {TRAP, {"leadsynth", "subbass", "syntharp", "pad", "vocal"}},
        {AMBIENT, {"pad", "piano", "syntharp", "violin", "cello"}},
        {NEW_AGE, {"pad", "piano", "syntharp", "flute", "sitar"}},
        {LATIN, {"guitar", "piano", "trumpet", "saxophone", "vocal", "marimba"}},
        {REGGAE, {"guitar", "organ", "piano", "vocal", "bass"}},
        {REGGAETON, {"guitar", "piano", "vocal", "leadsynth", "subbass"}},
        {RNB, {"piano", "vocal", "guitar", "syntharp", "bass"}},
        {SOUL, {"piano", "vocal", "organ", "guitar", "saxophone"}},
        {FUNK, {"guitar", "bass", "organ", "saxophone", "trumpet", "vocal"}},
        {DISCO, {"guitar", "bass", "piano", "strings", "vocal"}},
        {GOSPEL, {"piano", "organ", "vocal", "guitar"}},
        {WORLD, {"sitar", "marimba", "xylophone", "flute", "guitar", "vocal"}}
    };

    // Select instrument
    const auto& instruments = genreInstruments.at(g);
    melody.instrument = instruments[static_cast<size_t>(rng.random_L() * instruments.size())];

    // Set effects and automation
    melody.pan = (rng.random_L() < 0.5L) ? 0.3L : -0.3L;
    melody.reverbMix = (g == AMBIENT || g == CLASSICAL || g == NEW_AGE) ? 0.5L :
                       (g == JAZZ || g == BLUES || g == SOUL || g == CLASSICAL_JAZZ_FUSION) ? 0.35L :
                       (g == EDM || g == TECHNO || g == DUBSTEP || g == ELECTRONICA) ? 0.3L : 0.2L;
    melody.sectionName = "Melody";
    melody.useReverb = (g == AMBIENT || g == CLASSICAL || g == JAZZ || g == SOUL || g == EDM || g == NEW_AGE || g == CLASSICAL_JAZZ_FUSION || rng.random_L() < 0.5L);
    melody.reverbDelay = (g == AMBIENT || g == NEW_AGE) ? 0.1L : 0.05L;
    melody.reverbDecay = (g == AMBIENT || g == CLASSICAL || g == NEW_AGE) ? 0.6L : 0.4L;
    melody.reverbMix = melody.reverbMix;
    melody.useDistortion = (g == ROCK || g == METAL || g == PUNK || g == DUBSTEP) ? true : (rng.random_L() < 0.3333L);
    melody.distortionDrive = (g == METAL || g == DUBSTEP) ? 2.0L : 1.5L;
    melody.distortionThreshold = (g == METAL || g == DUBSTEP) ? 0.8L : 0.7L;

    // Genre-specific probabilities
    const long double restProb = (g == CLASSICAL || g == AMBIENT || g == NEW_AGE) ? 0.4L :
                                 (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION) ? 0.3L :
                                 (g == ROCK || g == METAL || g == PUNK) ? 0.2L :
                                 (g == LATIN || g == REGGAE || g == REGGAETON) ? 0.25L :
                                 (g == RNB || g == SOUL || g == GOSPEL || g == FUNK) ? 0.25L :
                                 (g == POP || g == COUNTRY || g == INDIE || g == FOLK || g == BLUEGRASS) ? 0.2L :
                                 (g == HIPHOP || g == RAP || g == TRAP) ? 0.2L :
                                 (g == EDM || g == TECHNO || g == DUBSTEP || g == ELECTRONICA) ? 0.15L :
                                 (g == WORLD) ? 0.3L : 0.25L;
    const long double ornamentProb = (g == CLASSICAL || g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION) ? 0.15L :
                                    (g == SOUL || g == GOSPEL || g == RNB) ? 0.1L :
                                    (g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD) ? 0.12L :
                                    (g == POP || g == COUNTRY || g == INDIE || g == FOLK) ? 0.08L :
                                    (g == ROCK || g == METAL || g == PUNK) ? 0.05L : 0.05L;
    const long double motifProb = (g == CLASSICAL || g == POP || g == ROCK || g == EDM || g == TECHNO || g == DUBSTEP) ? 0.4L :
                                 (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION) ? 0.35L :
                                 (g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD) ? 0.3L :
                                 (g == RNB || g == SOUL || g == GOSPEL || g == FUNK) ? 0.3L :
                                 (g == HIPHOP || g == RAP || g == TRAP) ? 0.25L : 0.3L;
    const long double arpeggioProb = (g == ROCK || g == POP || g == COUNTRY || g == INDIE) ? 0.15L :
                                     (g == CLASSICAL || g == JAZZ || g == EDM || g == TECHNO || g == CLASSICAL_JAZZ_FUSION) ? 0.35L :
                                     (g == AMBIENT || g == NEW_AGE) ? 0.25L :
                                     (g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD) ? 0.3L :
                                     (g == RNB || g == SOUL || g == GOSPEL || g == FUNK) ? 0.2L : 0.2L;

    melody.notes.reserve(500);
    melody.panAutomation.reserve(36);
    melody.volumeAutomation.reserve(36);
    melody.reverbMixAutomation.reserve(36);

    size_t invalidFreqCount = 0;
    const size_t maxInvalidFreqs = 100;

    // Generate automation for dynamic changes
    for (const auto& section : sections) {
        long double t = section.startTime;
        long double end = section.endTime;
        long double step = (end - t) / 4.0L;
        long double baseVol = (section.templateName == "Chorus" || section.templateName == "Drop") ? 0.6L :
                              (section.templateName == "Intro" || section.templateName == "Outro") ? 0.3L : 0.4L;
        for (int i = 0; i < 4 && t < end; ++i) {
            long double pan = std::clamp(melody.pan + 0.1L * (rng.random_L() - 0.5L), -1.0L, 1.0L);
            long double vol = std::clamp(baseVol + 0.1L * rng.random_L(), baseVol, 1.0L);
            long double rev = std::clamp(melody.reverbMix + 0.05L * rng.random_L(), 0.0L, 1.0L);
            melody.panAutomation.emplace_back(t, pan);
            melody.volumeAutomation.emplace_back(t, vol);
            melody.reverbMixAutomation.emplace_back(t, rev);
            t += step;
        }
    }

    const auto& intervals = scales.at(scaleName);
    size_t currentIdx = static_cast<size_t>(rng.random_L() * intervals.size());
    long double currentFreq = getClosestFreq(rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L));
    std::vector<long double> stepProbs = (g == POP || g == ROCK || g == COUNTRY || g == SOUL || g == GOSPEL || g == RNB || g == BLUEGRASS || g == FOLK) ?
                                         std::vector<long double>{0.5L, 0.3L, 0.15L, 0.05L} :
                                         (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION) ? std::vector<long double>{0.3L, 0.3L, 0.25L, 0.15L} :
                                         (g == CLASSICAL || g == NEW_AGE) ? std::vector<long double>{0.35L, 0.35L, 0.2L, 0.1L} :
                                         (g == EDM || g == TECHNO || g == DUBSTEP || g == ELECTRONICA) ? std::vector<long double>{0.4L, 0.3L, 0.2L, 0.1L} :
                                         (g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD) ? std::vector<long double>{0.45L, 0.3L, 0.15L, 0.1L} :
                                         (g == HIPHOP || g == RAP || g == TRAP) ? std::vector<long double>{0.4L, 0.3L, 0.2L, 0.1L} :
                                         std::vector<long double>{0.5L, 0.3L, 0.15L, 0.05L};
    long double stepDist = rng.dist(stepProbs.begin(), stepProbs.end());
    const long double chromaticProb = (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION) ? 0.3L :
                                     (g == ROCK || g == METAL || g == PUNK) ? 0.1L :
                                     (g == CLASSICAL || g == SOUL || g == GOSPEL || g == RNB) ? 0.15L :
                                     (g == EDM || g == TECHNO || g == DUBSTEP || g == ELECTRONICA) ? 0.2L :
                                     (g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD) ? 0.25L :
                                     (g == HIPHOP || g == RAP || g == TRAP) ? 0.2L : 0.05L;

    auto progressions = getChordProgressions(scaleName, g);
    std::vector<int> chordProg = progressions[static_cast<size_t>(rng.random_L() * progressions.size())];
    std::vector<Note> melodyMotif = generateMotif(g, scaleName, rootFreq, bpm);

    for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
        const auto& section = sections[sectionIdx];
        std::string templateName = section.templateName;

        // Reuse melody template if available
        if (sectionTemplates.find(templateName + "_Melody") != sectionTemplates.end() &&
            (templateName == "Verse" || templateName == "Chorus" || templateName == "Drop" || templateName == "Head")) {
            long double intensity = (section.name.find("Chorus") != std::string::npos || section.name.find("Drop") != std::string::npos ||
                                     section.name.find("2") != std::string::npos) ? 1.2L : 1.0L;
            bool transpose = (section.name.find("2") != std::string::npos && rng.random_L() < 0.5L);
            long double transposeSemitones = transpose ? 2.0L : 0.0L;
            Part varied = varyPart(sectionTemplates[templateName + "_Melody"], section.startTime, g, intensity, transpose, transposeSemitones, scaleName);
            melody.notes.insert(melody.notes.end(), varied.notes.begin(), varied.notes.end());
            melody.panAutomation.insert(melody.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
            melody.volumeAutomation.insert(melody.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
            melody.reverbMixAutomation.insert(melody.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
            ::SDL_Log("Reused melody template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
            continue;
        }

        long double t = section.startTime;
        long double sectionEnd = section.endTime;
        long double sectionDur = sectionEnd - t;
        size_t maxNotes = static_cast<size_t>(sectionDur * (
            g == ROCK || g == EDM || g == TECHNO || g == DUBSTEP || g == PUNK ? 5.0L :
            g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == LATIN || g == REGGAE || g == REGGAETON ? 4.0L :
            g == AMBIENT || g == CLASSICAL || g == NEW_AGE ? 2.0L :
            g == RNB || g == SOUL || g == GOSPEL || g == FUNK ? 3.0L :
            g == HIPHOP || g == RAP || g == TRAP ? 4.0L : 3.0L));
        size_t sectionNoteCount = 0;
        long double phraseDur = 4.0L * (60.0L / bpm);
        long double phraseStart = t;
        size_t chordIdx = 0;

        while (t < sectionEnd && sectionNoteCount < maxNotes) {
            if (invalidFreqCount >= maxInvalidFreqs) {
                ::SDL_Log("Aborting melody generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                break;
            }
            if (rng.random_L() < restProb) {
                t += getRandomDuration(g, section.progress, bpm);
                t = snapToBeatGrid(t, bpm);
                continue;
            }

            bool useMotif = (rng.random_L() < motifProb) && (t + phraseDur <= sectionEnd);
            if (useMotif) {
                for (const auto& motifNote : melodyMotif) {
                    if (sectionNoteCount >= maxNotes || t + motifNote.startTime >= sectionEnd) break;
                    Note note = motifNote;
                    note.startTime = snapToBeatGrid(t + motifNote.startTime, bpm);
                    note.duration = std::min(motifNote.duration, sectionEnd - note.startTime);
                    note.volume = 0.4L + 0.2L * section.progress;
                    note.velocity = 0.8L + 0.2L * rng.random_L();
                    note.phoneme = melody.instrument.find("vocal") != std::string::npos ? static_cast<int>(rng.random_L() * 7) : -1;
                    note.open = melody.instrument.find("hihat") != std::string::npos ? (rng.random_L() < 0.3333L) : false;
                    auto chord = buildChord(chordProg[chordIdx % chordProg.size()], scaleName, rootFreq, g);
                    if (!chord.empty()) {
                        note.freq = chord[static_cast<size_t>(rng.random_L() * chord.size())];
                        if (!std::isfinite(note.freq)) {
                            note.freq = currentFreq;
                            invalidFreqCount++;
                        } else {
                            currentFreq = note.freq;
                        }
                    }
                    melody.notes.push_back(note);
                    sectionNoteCount++;
                }
                t += phraseDur;
                t = snapToBeatGrid(t, bpm);
                chordIdx++;
                continue;
            }

            Note note;
            note.startTime = snapToBeatGrid(t, bpm);
            note.duration = getRandomDuration(g, section.progress, bpm);
            if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
            if (!std::isfinite(note.duration) || note.duration <= 0.0L) {
                note.duration = (60.0L / bpm) / (
                    g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == LATIN || g == REGGAE || g == REGGAETON ? 3.0L :
                    g == AMBIENT || g == CLASSICAL || g == NEW_AGE ? 2.0L : 4.0L);
            }
            note.volume = 0.4L + 0.2L * section.progress;
            note.velocity = (t == phraseStart || t == snapToBeatGrid(phraseStart + 2.0L * (60.0L / bpm), bpm)) ? 0.9L : 0.7L + 0.2L * rng.random_L();
            note.phoneme = melody.instrument.find("vocal") != std::string::npos ? static_cast<int>(rng.random_L() * 7) : -1;
            note.open = melody.instrument.find("hihat") != std::string::npos ? (rng.random_L() < 0.3333L) : false;

            if (rng.random_L() < ornamentProb && note.duration > 0.125L) {
                Note ornamentNote = note;
                ornamentNote.duration = note.duration * 0.25L;
                ornamentNote.startTime = note.startTime - ornamentNote.duration;
                int dir = (rng.random_L() < 0.5L) ? 1 : -1;
                currentIdx = (currentIdx + dir + intervals.size()) % intervals.size();
                ornamentNote.freq = getClosestFreq(rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L));
                ornamentNote.volume *= 0.7L;
                if (std::isfinite(ornamentNote.freq) && ornamentNote.startTime >= section.startTime) {
                    melody.notes.push_back(ornamentNote);
                    sectionNoteCount++;
                }
            }

            if (rng.random_L() < arpeggioProb) {
                auto chord = buildChord(chordProg[chordIdx % chordProg.size()], scaleName, rootFreq, g, rng.random_L() < 0.5L ? 1.0L : 0.0L);
                if (chord.empty() || !std::all_of(chord.begin(), chord.end(), [](long double f) { return std::isfinite(f); })) {
                    ::SDL_Log("Invalid chord frequencies in melody, using current freq");
                    note.freq = currentFreq;
                    invalidFreqCount++;
                } else {
                    note.freq = chord[static_cast<size_t>(rng.random_L() * chord.size())];
                    currentFreq = note.freq;
                }
            } else if (rng.random_L() < chromaticProb) {
                int dir = (rng.random_L() < 0.5L) ? 1 : -1;
                size_t freqIdx = 0;
                for (size_t j = 0; j < availableFreqs.size(); ++j) {
                    if (std::abs(currentFreq - availableFreqs[j]) < 1e-3L) {
                        freqIdx = j;
                        break;
                    }
                }
                freqIdx = (freqIdx + dir + availableFreqs.size()) % availableFreqs.size();
                note.freq = availableFreqs[freqIdx];
                if (!std::isfinite(note.freq)) {
                    note.freq = currentFreq;
                    invalidFreqCount++;
                }
                currentFreq = note.freq;
            } else {
                int step = static_cast<int>(stepDist);
                int dir = (rng.random_L() < 0.5L) ? 1 : -1;
                currentIdx = (currentIdx + dir * (step + 1)) % intervals.size();
                currentFreq = getClosestFreq(rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L));
                note.freq = currentFreq;
                if (!std::isfinite(note.freq)) {
                    note.freq = currentFreq;
                    invalidFreqCount++;
                }
            }

            melody.notes.push_back(note);
            sectionNoteCount++;
            t += note.duration;
            t = snapToBeatGrid(t, bpm);

            if (t >= phraseStart + phraseDur) {
                chordIdx++;
                phraseStart = t;
                if (rng.random_L() < 0.5L && t + (60.0L / bpm) <= sectionEnd) {
                    Note endNote = note;
                    endNote.startTime = snapToBeatGrid(t, bpm);
                    endNote.duration = 60.0L / bpm;
                    endNote.volume *= 0.9L;
                    auto chord = buildChord(chordProg[chordIdx % chordProg.size()], scaleName, rootFreq, g);
                    if (!chord.empty()) {
                        endNote.freq = chord[0];
                        currentFreq = endNote.freq;
                    }
                    if (std::isfinite(endNote.freq)) {
                        melody.notes.push_back(endNote);
                        sectionNoteCount++;
                    }
                    t += endNote.duration;
                    t = snapToBeatGrid(t, bpm);
                }
            }
        }
        ::SDL_Log("Generated %zu notes for melody in section %s", sectionNoteCount, section.name.c_str());

        // Store melody template
        if (templateName == "Verse" || templateName == "Chorus" || templateName == "Drop" || templateName == "Head") {
            Part templatePart = melody;
            templatePart.notes.clear();
            templatePart.panAutomation.clear();
            templatePart.volumeAutomation.clear();
            templatePart.reverbMixAutomation.clear();
            for (const auto& note : melody.notes) {
                if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                    Note templateNote = note;
                    templateNote.startTime -= section.startTime;
                    templatePart.notes.push_back(templateNote);
                }
            }
            for (const auto& [time, value] : melody.panAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.panAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : melody.volumeAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : melody.reverbMixAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                }
            }
            sectionTemplates[templateName + "_Melody"] = templatePart;
            ::SDL_Log("Stored melody template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
        }
    }
    ::SDL_Log("Generated melody with total %zu notes, %zu invalid frequencies encountered", melody.notes.size(), invalidFreqCount);
    return melody;
}

Part generateRhythm(Genre g, long double totalDur, long double beat, long double bpm, const std::string& instrument, const std::vector<Section>& sections) {
    static thread_local AudioUtils::RandomGenerator rng;
    Part rhythm;
    rhythm.instrument = instrument;
    rhythm.pan = (g == ROCK && instrument == "snare") ? 0.2L :
                 (g == JAZZ && instrument == "hihat_closed") ? -0.1L :
                 (g == LATIN && instrument == "tambourine") ? 0.15L :
                 (g == REGGAE && instrument == "kick") ? -0.15L :
                 (g == AMBIENT || g == NEW_AGE || g == CLASSICAL) ? (rng.random_L() - 0.5L) * 0.4L : // Wider panning for spatial genres
                 (g == HIPHOP || g == RAP || g == TRAP) ? (rng.random_L() < 0.5L ? 0.1L : -0.1L) : 0.0L;
    rhythm.reverbMix = (g == ROCK || g == METAL || g == PUNK) ? 0.15L :
                       (g == AMBIENT || g == CLASSICAL || g == NEW_AGE) ? 0.4L :
                       (g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD) ? 0.25L :
                       (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == RNB || g == SOUL || g == GOSPEL || g == FUNK) ? 0.3L :
                       (g == EDM || g == TECHNO || g == DUBSTEP || g == ELECTRONICA || g == HIPHOP || g == RAP || g == TRAP) ? 0.25L : 0.2L;
    rhythm.sectionName = "Rhythm";
    rhythm.useReverb = (g == ROCK || g == METAL || g == AMBIENT || g == CLASSICAL || g == NEW_AGE || g == LATIN ||
                        g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == RNB || g == SOUL || g == GOSPEL ||
                        g == EDM || g == TECHNO || g == DUBSTEP || g == ELECTRONICA || rng.random_L() < 0.5L);
    rhythm.reverbDelay = (g == AMBIENT || g == NEW_AGE) ? 0.1L : 0.05L;
    rhythm.reverbDecay = (g == AMBIENT || g == CLASSICAL || g == NEW_AGE) ? 0.8L : 0.5L;
    rhythm.reverbMix = rhythm.reverbMix;
    rhythm.useDistortion = (g == ROCK && (instrument == "kick" || instrument == "snare")) ||
                           (g == METAL && (instrument == "kick" || instrument == "snare")) ||
                           (g == PUNK && instrument == "snare") ||
                           (g == DUBSTEP && instrument == "subbass") ||
                           (g == TRAP && (instrument == "kick" || instrument == "subbass")) ||
                           (rng.random_L() < 0.25L && g != CLASSICAL && g != AMBIENT && g == NEW_AGE);
    rhythm.distortionDrive = (g == METAL || g == DUBSTEP || g == TRAP) ? 1.5L : 1.2L;
    rhythm.distortionThreshold = (g == METAL || g == DUBSTEP || g == TRAP) ? 0.9L : 0.7L;

    const size_t maxNotesPerSection = 100;
    rhythm.notes.reserve(maxNotesPerSection * sections.size());
    rhythm.panAutomation.reserve(36);
    rhythm.volumeAutomation.reserve(36);
    rhythm.reverbMixAutomation.reserve(36);

    std::map<std::string, std::pair<long double, long double>> instrumentRanges = {
        {"kick", {40.0L, 100.0L}},
        {"snare", {150.0L, 250.0L}},
        {"cymbal", {200.0L, 1000.0L}},
        {"hihat_closed", {300.0L, 800.0L}},
        {"hihat_open", {300.0L, 800.0L}},
        {"clap", {200.0L, 600.0L}},
        {"subbass", {30.0L, 100.0L}},
        {"tambourine", {200.0L, 700.0L}},
        {"tom", {80.0L, 200.0L}},
        {"marimba", {100.0L, 400.0L}},
        {"xylophone", {200.0L, 800.0L}},
        {"bell", {300.0L, 1200.0L}},
        {"vocal", {100.0L, 800.0L}} // Added for compatibility with generateMelody
    };

    std::vector<long double> pattern;
    long double noteDur = beat * 0.5L;
    long double swingFactor = (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == FUNK || g == REGGAE) ? 0.67L :
                              (g == LATIN || g == REGGAETON) ? 0.75L :
                              (g == HIPHOP || g == RAP || g == TRAP) ? 0.7L :
                              (g == RNB || g == SOUL || g == GOSPEL) ? 0.65L : 1.0L;
    long double syncopationProb = (g == JAZZ || g == FUNK || g == LATIN || g == REGGAE || g == REGGAETON || g == HIPHOP || g == TRAP || g == CLASSICAL_JAZZ_FUSION) ? 0.5L :
                                 (g == RNB || g == SOUL || g == GOSPEL || g == DISCO) ? 0.4L :
                                 (g == POP || g == ROCK || g == COUNTRY || g == INDIE || g == FOLK || g == BLUEGRASS) ? 0.3L :
                                 (g == EDM || g == TECHNO || g == DUBSTEP || g == ELECTRONICA) ? 0.35L :
                                 (g == AMBIENT || g == CLASSICAL || g == NEW_AGE) ? 0.2L :
                                 (g == WORLD) ? 0.45L : 0.3L;

    switch (g) {
        case ROCK:
        case PUNK:
        case METAL:
            if (instrument == "kick") pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            else if (instrument == "snare") pattern = {1.0L, 3.0L};
            else if (instrument == "cymbal" || instrument == "hihat_closed") 
                pattern = {0.0L, 0.5L, 1.0L, 1.5L, 2.0L, 2.5L, 3.0L, 3.5L};
            else if (instrument == "tom") pattern = {0.5L, 2.5L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.5L;
            break;
        case JAZZ:
        case BLUES:
        case CLASSICAL_JAZZ_FUSION:
            if (instrument == "kick") pattern = {0.0L, 2.0L};
            else if (instrument == "snare") pattern = {1.0L, 3.0L};
            else if (instrument == "hihat_closed") 
                pattern = {0.0L, 0.67L, 1.0L, 1.67L, 2.0L, 2.67L, 3.0L, 3.67L};
            else if (instrument == "cymbal") pattern = {0.0L, 2.0L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.5L * swingFactor;
            break;
        case FUNK:
        case DISCO:
            if (instrument == "kick") pattern = {0.0L, 0.75L, 2.0L, 2.75L};
            else if (instrument == "snare") pattern = {1.0L, 1.5L, 3.0L};
            else if (instrument == "hihat_closed") 
                pattern = {0.0L, 0.25L, 0.5L, 0.75L, 1.0L, 1.25L, 1.5L, 1.75L, 2.0L, 2.25L, 2.5L, 2.75L, 3.0L, 3.25L, 3.5L, 3.75L};
            else if (instrument == "clap") pattern = {1.0L, 3.0L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.25L;
            break;
        case REGGAE:
        case REGGAETON:
            if (instrument == "kick") pattern = {1.0L, 3.0L};
            else if (instrument == "snare") pattern = {1.0L, 3.0L};
            else if (instrument == "hihat_closed") 
                pattern = {0.5L, 1.5L, 2.5L, 3.5L};
            else if (instrument == "tambourine") pattern = {0.5L, 1.5L, 2.5L, 3.5L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.5L;
            break;
        case LATIN:
            if (instrument == "kick") pattern = {0.0L, 1.5L, 2.0L, 3.5L};
            else if (instrument == "snare") pattern = {1.0L, 2.5L};
            else if (instrument == "hihat_closed") 
                pattern = {0.0L, 0.25L, 0.5L, 1.0L, 1.25L, 1.5L, 2.0L, 2.25L, 2.5L, 3.0L, 3.25L, 3.5L};
            else if (instrument == "tambourine") pattern = {0.25L, 1.25L, 2.25L, 3.25L};
            else if (instrument == "marimba") pattern = {0.5L, 1.5L, 2.5L, 3.5L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.25L;
            break;
        case EDM:
        case TECHNO:
        case DUBSTEP:
        case ELECTRONICA:
            if (instrument == "kick") pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            else if (instrument == "snare") pattern = {1.0L, 3.0L};
            else if (instrument == "hihat_closed") 
                pattern = {0.5L, 1.5L, 2.5L, 3.5L};
            else if (instrument == "subbass") pattern = {0.0L, 2.0L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.5L;
            break;
        case GOSPEL:
        case SOUL:
        case RNB:
            if (instrument == "kick") pattern = {0.0L, 2.0L, 2.5L};
            else if (instrument == "snare") pattern = {1.0L, 3.0L};
            else if (instrument == "clap") pattern = {1.0L, 3.0L};
            else if (instrument == "hihat_closed") pattern = {0.5L, 1.5L, 2.5L, 3.5L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.5L;
            break;
        case HIPHOP:
        case RAP:
        case TRAP:
            if (instrument == "kick") pattern = {0.0L, 0.75L, 2.0L};
            else if (instrument == "snare") pattern = {1.0L, 3.0L};
            else if (instrument == "hihat_closed") 
                pattern = {0.0L, 0.167L, 0.333L, 0.5L, 0.667L, 0.833L, 1.0L, 1.167L, 1.333L, 1.5L, 1.667L, 1.833L,
                           2.0L, 2.167L, 2.333L, 2.5L, 2.667L, 2.833L, 3.0L, 3.167L, 3.333L, 3.5L, 3.667L, 3.833L};
            else if (instrument == "subbass") pattern = {0.0L, 2.0L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.167L; // Support triplet-based rhythms
            break;
        case AMBIENT:
        case NEW_AGE:
        case CLASSICAL:
            if (instrument == "kick") pattern = {0.0L, 2.0L};
            else if (instrument == "bell") pattern = {0.0L, 2.0L};
            else if (instrument == "xylophone") pattern = {1.0L, 3.0L};
            else pattern = {0.0L, 2.0L};
            noteDur = beat * 1.0L;
            break;
        case WORLD:
            if (instrument == "kick") pattern = {0.0L, 2.0L};
            else if (instrument == "tambourine") pattern = {0.5L, 1.5L, 2.5L, 3.5L};
            else if (instrument == "marimba") pattern = {0.25L, 1.25L, 2.25L, 3.25L};
            else if (instrument == "xylophone") pattern = {1.0L, 3.0L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.5L;
            break;
        case COUNTRY:
        case BLUEGRASS:
        case FOLK:
        case POP:
        case INDIE:
            if (instrument == "kick") pattern = {0.0L, 2.0L};
            else if (instrument == "snare") pattern = {1.0L, 3.0L};
            else if (instrument == "hihat_closed") pattern = {0.5L, 1.5L, 2.5L, 3.5L};
            else pattern = {0.0L, 1.0L, 2.0L, 3.0L};
            noteDur = beat * 0.5L;
            break;
        default:
            if (instrument == "kick") pattern = {0.0L, 2.0L};
            else if (instrument == "snare") pattern = {1.0L, 3.0L};
            else pattern = {0.0L, 0.5L, 1.0L, 1.5L, 2.0L, 2.5L, 3.0L, 3.5L};
            noteDur = beat * 0.5L;
            break;
    }

    // Instrument-specific duration overrides
    if (instrument == "hihat_open") noteDur = beat * 1.5L;
    else if (instrument == "cymbal") noteDur = beat * 2.0L;
    else if (instrument == "bell") noteDur = beat * 1.0L;
    else if (instrument == "marimba" || instrument == "xylophone") noteDur = beat * 0.75L;

    // Generate automation
    for (const auto& section : sections) {
        long double t = section.startTime;
        long double end = section.endTime;
        long double step = (end - t) / 4.0L;
        long double baseVol = (section.templateName == "Chorus" || section.templateName == "Drop") ? 0.7L : 0.5L;
        long double baseRev = (section.templateName == "Outro" || g == AMBIENT || g == NEW_AGE) ? rhythm.reverbMix + 0.1L : rhythm.reverbMix;
        for (int i = 0; i < 4 && t < end; ++i) {
            long double pan = std::clamp(rhythm.pan + 0.1L * (rng.random_L() - 0.5L), -1.0L, 1.0L);
            long double vol = std::clamp(baseVol + 0.1L * rng.random_L(), 0.4L, 1.0L);
            long double rev = std::clamp(baseRev + 0.05L * rng.random_L(), 0.0L, 1.0L);
            rhythm.panAutomation.emplace_back(t, pan);
            rhythm.volumeAutomation.emplace_back(t, vol);
            rhythm.reverbMixAutomation.emplace_back(t, rev);
            t += step;
        }
    }

    for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
        const auto& section = sections[sectionIdx];
        std::string templateName = section.templateName;

        // Reuse rhythm template if available
        if (sectionTemplates.find(templateName + "_Rhythm_" + instrument) != sectionTemplates.end() &&
            (templateName == "Verse" || templateName == "Chorus" || templateName == "Drop" || templateName == "Head")) {
            long double intensity = (section.name.find("Chorus") != std::string::npos || section.name.find("Drop") != std::string::npos ||
                                     section.name.find("2") != std::string::npos) ? 1.2L : 1.0L;
            Part varied = varyPart(sectionTemplates[templateName + "_Rhythm_" + instrument], section.startTime, g, intensity);
            rhythm.notes.insert(rhythm.notes.end(), varied.notes.begin(), varied.notes.end());
            rhythm.panAutomation.insert(rhythm.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
            rhythm.volumeAutomation.insert(rhythm.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
            rhythm.reverbMixAutomation.insert(rhythm.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
            ::SDL_Log("Reused rhythm template %s_%s for section %s with %zu notes", templateName.c_str(), instrument.c_str(), section.name.c_str(), varied.notes.size());
            continue;
        }

        long double t = section.startTime;
        long double sectionEnd = section.endTime;
        size_t sectionNoteCount = 0;
        long double density = (section.templateName == "Intro" || section.templateName == "Outro") ? 0.5L :
                             (section.templateName == "Chorus" || section.templateName == "Drop") ? 1.2L : 1.0L;

        while (t < sectionEnd && sectionNoteCount < maxNotesPerSection) {
            for (long double offset : pattern) {
                if (t + offset * beat >= sectionEnd) break;
                if (sectionNoteCount >= maxNotesPerSection) break;
                if (rng.random_L() > density) continue;

                Note note;
                note.startTime = snapToBeatGrid(t + offset * beat * swingFactor, bpm);
                note.duration = noteDur;
                if (!std::isfinite(note.duration) || note.duration <= 0.0L) {
                    note.duration = beat * 0.25L;
                }

                auto it = instrumentRanges.find(instrument);
                long double freq = (it != instrumentRanges.end()) ?
                                   it->second.first + (it->second.second - it->second.first) * rng.random_L() :
                                   (instrument == "kick") ? 60.0L : (instrument == "snare") ? 200.0L : 400.0L;
                note.freq = std::clamp(freq, it != instrumentRanges.end() ? it->second.first : 40.0L,
                                       it != instrumentRanges.end() ? it->second.second : 1000.0L);

                note.volume = (section.templateName == "Chorus" || section.templateName == "Drop") ? 0.7L : 0.5L;
                note.velocity = (offset == 0.0L || offset == 1.0L || offset == 2.0L || offset == 3.0L) ? 0.9L : 0.7L;
                if (rng.random_L() < 0.2L) note.velocity *= 0.8L;
                note.open = (instrument == "hihat_open") || (instrument == "hihat_closed" && rng.random_L() < 0.1L);
                note.phoneme = -1;

                rhythm.notes.push_back(note);
                sectionNoteCount++;

                if (rng.random_L() < syncopationProb && offset < 3.5L * beat) {
                    Note syncNote = note;
                    syncNote.startTime = snapToBeatGrid(t + offset * beat * swingFactor + beat * (g == HIPHOP || g == RAP || g == TRAP ? 0.167L : 0.25L), bpm);
                    syncNote.velocity *= 0.8L;
                    if (syncNote.startTime < sectionEnd && sectionNoteCount < maxNotesPerSection) {
                        rhythm.notes.push_back(syncNote);
                        sectionNoteCount++;
                    }
                }
            }
            t += beat * 4.0L;
            t = snapToBeatGrid(t, bpm);
        }
        ::SDL_Log("Generated %zu notes for rhythm (%s) in section %s", sectionNoteCount, instrument.c_str(), section.name.c_str());

        // Store rhythm template
        if (templateName == "Verse" || templateName == "Chorus" || templateName == "Drop" || templateName == "Head") {
            Part templatePart = rhythm;
            templatePart.notes.clear();
            templatePart.panAutomation.clear();
            templatePart.volumeAutomation.clear();
            templatePart.reverbMixAutomation.clear();
            for (const auto& note : rhythm.notes) {
                if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                    Note templateNote = note;
                    templateNote.startTime -= section.startTime;
                    templatePart.notes.push_back(templateNote);
                }
            }
            for (const auto& [time, value] : rhythm.panAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.panAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : rhythm.volumeAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : rhythm.reverbMixAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                }
            }
            sectionTemplates[templateName + "_Rhythm_" + instrument] = templatePart;
            ::SDL_Log("Stored rhythm template %s_%s with %zu notes", templateName.c_str(), instrument.c_str(), templatePart.notes.size());
        }
    }
    ::SDL_Log("Generated rhythm with total %zu notes for instrument %s", rhythm.notes.size(), instrument.c_str());
    return rhythm;
}

    Part generateSaxophone(Genre g, const std::string& scaleName, long double rootFreq, long double totalDur, const std::vector<Section>& sections, long double bpm) {
        static thread_local AudioUtils::RandomGenerator rng;
        ::SDL_Log("Generating saxophone for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
        
        Part saxophone;
        saxophone.instrument = (rng.random_L() < 0.2L) ? "saxophone_bright" :
                               (rng.random_L() < 0.4L) ? "saxophone_muted" : "saxophone";
        
        // Set initial parameters
        saxophone.pan = (rng.random_L() < 0.5L) ? 0.2L : -0.2L;
        saxophone.reverbMix = (g == JAZZ || g == BLUES || g == SOUL || g == CLASSICAL_JAZZ_FUSION) ? 0.3L :
                              (g == AMBIENT || g == NEW_AGE) ? 0.4L :
                              (g == FUNK || g == LATIN || g == RNB) ? 0.25L : 0.2L;
        saxophone.sectionName = "Saxophone";
        saxophone.useReverb = (g == JAZZ || g == BLUES || g == SOUL || g == AMBIENT || g == CLASSICAL_JAZZ_FUSION ||
                               g == NEW_AGE || g == LATIN || rng.random_L() < 0.5L);
        saxophone.reverbDelay = 0.12L;
        saxophone.reverbDecay = 0.6L;
        saxophone.reverbMixFactor = saxophone.reverbMix;
        saxophone.useDistortion = (g == FUNK || g == ROCK || g == BLUES || rng.random_L() < 0.25L);
        saxophone.distortionDrive = saxophone.instrument == "saxophone_bright" ? 1.8L : 
                                    saxophone.instrument == "saxophone_muted" ? 1.4L : 1.6L;
        saxophone.distortionThreshold = 0.75L;

        // Probabilities for note types
        const long double restProb = (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION) ? 0.45L :
                                     (g == FUNK || g == SOUL || g == RNB || g == GOSPEL) ? 0.35L :
                                     (g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD) ? 0.3L :
                                     (g == POP || g == ROCK || g == COUNTRY || g == INDIE || g == FOLK || g == BLUEGRASS) ? 0.25L :
                                     (g == AMBIENT || g == NEW_AGE || g == CLASSICAL) ? 0.4L : 0.3L;
        const long double legatoProb = (g == JAZZ || g == BLUES || g == SOUL || g == CLASSICAL_JAZZ_FUSION || g == RNB) ? 0.6L :
                                      (g == AMBIENT || g == NEW_AGE || g == CLASSICAL) ? 0.5L :
                                      (g == LATIN || g == WORLD) ? 0.4L : 0.3L;
        const long double stabProb = (g == FUNK || g == LATIN || g == REGGAE || g == REGGAETON) ? 0.5L :
                                    (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == RNB) ? 0.3L :
                                    (g == POP || g == ROCK) ? 0.25L : 0.2L;
        const long double improvProb = (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION) ? 0.4L :
                                      (g == SOUL || g == RNB || g == GOSPEL) ? 0.25L :
                                      (g == LATIN || g == WORLD) ? 0.2L : 0.1L;

        // Reserve space
        saxophone.notes.reserve(400);
        saxophone.panAutomation.reserve(36);
        saxophone.volumeAutomation.reserve(36);
        saxophone.reverbMixAutomation.reserve(36);

        size_t invalidFreqCount = 0;
        const size_t maxInvalidFreqs = 100;

        // Automation
        for (const auto& section : sections) {
            long double t = section.startTime;
            long double end = section.endTime;
            long double step = (end - t) / 4.0L;
            for (int i = 0; i < 4 && t < end; ++i) {
                long double pan = std::clamp(saxophone.pan + 0.1L * (rng.random_L() - 0.5L), -1.0L, 1.0L);
                long double vol = std::clamp(0.45L + 0.15L * rng.random_L(), 0.45L, 1.0L);
                long double rev = std::clamp(saxophone.reverbMix + 0.1L * rng.random_L(), 0.0L, 1.0L);
                saxophone.panAutomation.emplace_back(t, pan);
                saxophone.volumeAutomation.emplace_back(t, vol);
                saxophone.reverbMixAutomation.emplace_back(t, rev);
                t += step;
            }
        }

        const auto& intervals = scales.at(scaleName);
        size_t currentIdx = static_cast<size_t>(rng.random_L() * intervals.size());
        long double currentFreq = getClosestFreq(rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L), saxFreqs);

        for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
            const auto& section = sections[sectionIdx];
            std::string templateName = section.templateName;

            // Reuse templates
            if (sectionTemplates.find(templateName + "_Saxophone_" + saxophone.instrument) != sectionTemplates.end() &&
                (templateName == "Verse" || templateName == "Chorus" || templateName == "Solo")) {
                long double intensity = (section.name.find("Chorus2") != std::string::npos || section.name.find("Solo") != std::string::npos ||
                                         section.name.find("Verse2") != std::string::npos) ? 1.15L : 1.0L;
                bool transpose = (section.name.find("Chorus2") != std::string::npos && rng.random_L() < 0.3333L);
                long double transposeSemitones = transpose ? 2.0L : 0.0L;
                Part varied = varyPart(sectionTemplates[templateName + "_Saxophone_" + saxophone.instrument], section.startTime, g, intensity, transpose, transposeSemitones, scaleName);
                saxophone.notes.insert(saxophone.notes.end(), varied.notes.begin(), varied.notes.end());
                saxophone.panAutomation.insert(saxophone.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                saxophone.volumeAutomation.insert(saxophone.panAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                saxophone.reverbMixAutomation.insert(saxophone.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                ::SDL_Log("Reused saxophone template %s_%s for section %s with %zu notes", templateName.c_str(), saxophone.instrument.c_str(), section.name.c_str(), varied.notes.size());
                continue;
            }

            long double t = section.startTime;
            long double sectionEnd = section.endTime;
            long double sectionDur = sectionEnd - t;
            size_t maxNotes = static_cast<size_t>(sectionDur * (
                g == JAZZ || g == BLUES || g == FUNK || g == CLASSICAL_JAZZ_FUSION ? 3.5L :
                g == AMBIENT || g == NEW_AGE || g == CLASSICAL ? 2.0L :
                g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD ? 3.0L : 2.5L));
            size_t sectionNoteCount = 0;
            long double phraseDur = 4.0L * (60.0L / bpm);
            long double phraseStart = t;

            // Get chord progression
            std::vector<int> prog;
            if (chordProgressions.find(templateName) != chordProgressions.end()) {
                prog = chordProgressions[templateName];
            } else {
                auto progs = getChordProgressions(scaleName, g);
                prog = progs[static_cast<size_t>(rng.random_L() * progs.size())];
                if (templateName == "Verse" || templateName == "Chorus" || templateName == "Solo") {
                    chordProgressions[templateName] = prog;
                }
            }
            size_t chordIdx = 0;

            while (t < sectionEnd && sectionNoteCount < maxNotes) {
                if (invalidFreqCount >= maxInvalidFreqs) {
                    ::SDL_Log("Aborting saxophone generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                    break;
                }
                if (rng.random_L() < restProb) {
                    t += getRandomDuration(g, section.progress, bpm);
                    t = snapToBeatGrid(t, bpm);
                    continue;
                }

                bool useLegato = rng.random_L() < legatoProb;
                bool useStab = !useLegato && rng.random_L() < stabProb;
                bool useImprov = !useLegato && !useStab && rng.random_L() < improvProb;

                if (useStab) {
                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = 60.0L / (bpm * 4.0L);
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.0625L;

                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                    long double targetFreq = chord[static_cast<size_t>(rng.random_L() * chord.size())];
                    while (targetFreq > 1500.0L) targetFreq /= 2.0L;
                    while (targetFreq < 110.0L) targetFreq *= 2.0L;
                    note.freq = getClosestFreq(targetFreq, saxFreqs);

                    if (!std::isfinite(note.freq)) {
                        ::SDL_Log("Invalid saxophone frequency at t=%.2Lf, using 110.0 Hz", t);
                        note.freq = 110.0L;
                        invalidFreqCount++;
                    } else {
                        currentFreq = note.freq;
                    }
                    note.volume = 0.5L + 0.1L * section.progress;
                    note.velocity = saxophone.instrument == "saxophone_muted" ? 0.8L : 0.9L;
                    note.phoneme = -1;
                    note.open = false;
                    saxophone.notes.push_back(note);
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);
                    sectionNoteCount++;
                    chordIdx++;
                } else if (useImprov && (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == RNB || g == SOUL || g == GOSPEL)) {
                    int numNotes = 3 + static_cast<int>(rng.random_L() * 3);
                    long double runDur = 60.0L / (bpm * 2.0L);
                    long double noteDur = runDur / static_cast<long double>(numNotes);

                    for (int i = 0; i < numNotes && t < sectionEnd && sectionNoteCount < maxNotes; ++i) {
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = noteDur;
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.0625L;

                        int step = (rng.random_L() < 0.5L) ? 1 : -1;
                        currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                        long double targetFreq = rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L);
                        while (targetFreq > 1500.0L) targetFreq /= 2.0L;
                        while (targetFreq < 110.0L) targetFreq *= 2.0L;
                        note.freq = getClosestFreq(targetFreq, saxFreqs);
                        currentFreq = note.freq;

                        if (!std::isfinite(note.freq)) {
                            ::SDL_Log("Invalid saxophone frequency at t=%.2Lf, using 110.0 Hz", t);
                            note.freq = 110.0L;
                            invalidFreqCount++;
                        }
                        note.volume = 0.45L + 0.1L * section.progress;
                        note.velocity = 0.7L + 0.2L * rng.random_L();
                        if (saxophone.instrument == "saxophone_muted") note.velocity *= 0.9L;
                        note.phoneme = -1;
                        note.open = false;
                        saxophone.notes.push_back(note);
                        t += note.duration;
                        sectionNoteCount++;
                    }
                    t = snapToBeatGrid(t, bpm);
                } else {
                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = useLegato ? getRandomDuration(g, section.progress, bpm) * 1.5L : getRandomDuration(g, section.progress, bpm);
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.0625L;

                    int step = static_cast<int>(rng.random_L() * 3) - 1;
                    currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                    long double targetFreq = rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L);
                    while (targetFreq > 1500.0L) targetFreq /= 2.0L;
                    while (targetFreq < 110.0L) targetFreq *= 2.0L;
                    currentFreq = note.freq = getClosestFreq(targetFreq, saxFreqs);

                    if (!std::isfinite(note.freq)) {
                        ::SDL_Log("Invalid saxophone frequency at t=%.2Lf, using 110.0 Hz", t);
                        note.freq = 110.0L;
                        invalidFreqCount++;
                    }
                    note.volume = 0.45L + 0.1L * section.progress;
                    note.velocity = useLegato ? 0.7L + 0.15L * rng.random_L() : 0.85L + 0.15L * rng.random_L();
                    if (saxophone.instrument == "saxophone_muted") note.velocity *= 0.9L;
                    note.phoneme = -1;
                    note.open = false;
                    saxophone.notes.push_back(note);
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);
                    sectionNoteCount++;

                    if (useLegato && rng.random_L() < 0.3333L && note.duration > 0.125L) {
                        Note graceNote = note;
                        graceNote.duration = note.duration * 0.2L;
                        graceNote.startTime = note.startTime - graceNote.duration;
                        graceNote.volume *= 0.6L;
                        graceNote.velocity *= 0.8L;
                        if (graceNote.startTime >= section.startTime) {
                            saxophone.notes.push_back(graceNote);
                            sectionNoteCount++;
                        }
                    }
                }

                if (t >= phraseStart + phraseDur) {
                    phraseStart = t;
                    if (rng.random_L() < 0.5L) {
                        t += 60.0L / bpm;
                        t = snapToBeatGrid(t, bpm);
                    }
                    chordIdx++;
                }
            }
            ::SDL_Log("Generated %zu notes for saxophone in section %s", sectionNoteCount, section.name.c_str());

            // Store template
            if (templateName == "Verse" || templateName == "Chorus" || templateName == "Solo") {
                Part templatePart = saxophone;
                templatePart.notes.clear();
                templatePart.panAutomation.clear();
                templatePart.volumeAutomation.clear();
                templatePart.reverbMixAutomation.clear();
                for (const auto& note : saxophone.notes) {
                    if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                        Note templateNote = note;
                        templateNote.startTime -= section.startTime;
                        templatePart.notes.push_back(templateNote);
                    }
                }
                for (const auto& [time, value] : saxophone.panAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.panAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : saxophone.volumeAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : saxophone.reverbMixAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                sectionTemplates[templateName + "_Saxophone_" + saxophone.instrument] = templatePart;
                ::SDL_Log("Stored saxophone template %s_%s with %zu notes", templateName.c_str(), saxophone.instrument.c_str(), templatePart.notes.size());
            }
        }
        ::SDL_Log("Generated saxophone with total %zu notes, %zu invalid frequencies encountered", saxophone.notes.size(), invalidFreqCount);
        return saxophone;
    }

    Part generatePiano(Genre g, const std::string& scaleName, long double rootFreq, long double totalDur, const std::vector<Section>& sections, long double bpm) {
        static thread_local AudioUtils::RandomGenerator rng;
        ::SDL_Log("Generating piano for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
        
        Part piano;
        piano.instrument = (rng.random_L() < 0.2L) ? "piano_bright" :
                           (rng.random_L() < 0.4L) ? "piano_soft" : "piano";
        
        // Set initial parameters
        piano.pan = (rng.random_L() < 0.5L) ? 0.15L : -0.15L;
        piano.reverbMix = (g == CLASSICAL || g == AMBIENT || g == NEW_AGE) ? 0.5L :
                          (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == RNB || g == SOUL || g == GOSPEL) ? 0.35L :
                          (g == POP || g == ROCK || g == INDIE || g == COUNTRY || g == FOLK || g == BLUEGRASS) ? 0.25L : 0.3L;
        piano.sectionName = "Piano";
        piano.useReverb = (g == CLASSICAL || g == AMBIENT || g == NEW_AGE || g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION ||
                           g == RNB || g == SOUL || g == GOSPEL || g == POP || rng.random_L() < 0.6L);
        piano.reverbDelay = 0.6L;
        piano.reverbDecay = 0.8L;
        piano.useDistortion = (g == ROCK || g == BLUES || g == FUNK || rng.random_L() < 0.2L);
        piano.distortionDrive = piano.instrument == "piano_bright" ? 1.7L :
                                piano.instrument == "piano_soft" ? 1.3L : 1.5L;
        piano.distortionThreshold = 0.9L;

        // Probabilities for note types
        const long double restProb = (g == CLASSICAL || g == AMBIENT || g == NEW_AGE) ? 0.4L :
                                     (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION) ? 0.35L :
                                     (g == POP || g == ROCK || g == INDIE || g == COUNTRY || g == FOLK || g == BLUEGRASS) ? 0.25L :
                                     (g == RNB || g == SOUL || g == GOSPEL || g == FUNK) ? 0.3L :
                                     (g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD) ? 0.28L : 0.3L;
        const long double chordProb = (g == CLASSICAL || g == JAZZ || g == CLASSICAL_JAZZ_FUSION || g == GOSPEL) ? 0.6L :
                                     (g == POP || g == ROCK || g == INDIE || g == COUNTRY || g == FOLK || g == BLUEGRASS) ? 0.4L :
                                     (g == RNB || g == SOUL || g == FUNK) ? 0.5L :
                                     (g == AMBIENT || g == NEW_AGE) ? 0.3L : 0.45L;
        const long double arpeggioProb = (g == CLASSICAL || g == AMBIENT || g == NEW_AGE) ? 0.5L :
                                        (g == JAZZ || g == CLASSICAL_JAZZ_FUSION || g == LATIN) ? 0.3L :
                                        (g == POP || g == EDM || g == ELECTRONICA) ? 0.25L : 0.2L;
        const long double melodyProb = (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == RNB || g == SOUL || g == GOSPEL) ? 0.4L :
                                      (g == POP || g == ROCK || g == INDIE || g == COUNTRY || g == FOLK || g == BLUEGRASS) ? 0.35L :
                                      (g == CLASSICAL || g == AMBIENT || g == NEW_AGE) ? 0.3L :
                                      (g == LATIN || g == REGGAE || g == WORLD) ? 0.25L : 0.3L;

        // Reserve space
        piano.notes.reserve(600);
        piano.panAutomation.reserve(36);
        piano.volumeAutomation.reserve(36);
        piano.reverbMixAutomation.reserve(36);

        size_t invalidFreqCount = 0;
        const size_t maxInvalidFreqs = 100;

        // Automation
        for (const auto& section : sections) {
            long double t = section.startTime;
            long double end = section.endTime;
            long double step = (end - t) / 4.0L;
            for (int i = 0; i < 4 && t < end; ++i) {
                long double pan = std::clamp(piano.pan + 0.1L * (rng.random_L() - 0.5L), -1.0L, 1.0L);
                long double vol = std::clamp(0.5L + 0.15L * rng.random_L(), 0.5L, 1.0L);
                long double rev = std::clamp(piano.reverbMix + 0.1L * rng.random_L(), 0.0L, 1.0L);
                piano.panAutomation.emplace_back(t, pan);
                piano.volumeAutomation.emplace_back(t, vol);
                piano.reverbMixAutomation.emplace_back(t, rev);
                t += step;
            }
        }

        const auto& intervals = scales.at(scaleName);
        size_t currentIdx = static_cast<size_t>(rng.random_L() * intervals.size());
        long double currentFreq = getClosestFreq(rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L), pianoFreqs);

        for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
            const auto& section = sections[sectionIdx];
            std::string templateName = section.templateName;

            // Reuse templates
            if (sectionTemplates.find(templateName + "_Piano_" + piano.instrument) != sectionTemplates.end() &&
                (templateName == "Verse" || templateName == "Chorus" || templateName == "Bridge" || templateName == "Intro" || templateName == "Outro")) {
                long double intensity = (section.name.find("Chorus") != std::string::npos || section.name.find("Bridge") != std::string::npos ||
                                         section.name.find("2") != std::string::npos) ? 1.2L : 1.0L;
                bool transpose = (section.name.find("Chorus2") != std::string::npos && rng.random_L() < 0.3L);
                long double transposeSemitones = transpose ? 2.0L : 0.0L;
                Part varied = varyPart(sectionTemplates[templateName + "_Piano_" + piano.instrument], section.startTime, g, intensity, transpose, transposeSemitones, scaleName);
                piano.notes.insert(piano.notes.end(), varied.notes.begin(), varied.notes.end());
                piano.panAutomation.insert(piano.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                piano.volumeAutomation.insert(piano.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                piano.reverbMixAutomation.insert(piano.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                ::SDL_Log("Reused piano template %s_%s for section %s with %zu notes", templateName.c_str(), piano.instrument.c_str(), section.name.c_str(), varied.notes.size());
                continue;
            }

            long double t = section.startTime;
            long double sectionEnd = section.endTime;
            long double sectionDur = sectionEnd - t;
            size_t maxNotes = static_cast<size_t>(sectionDur * (
                g == CLASSICAL || g == JAZZ || g == CLASSICAL_JAZZ_FUSION ? 4.0L :
                g == AMBIENT || g == NEW_AGE ? 2.0L :
                g == POP || g == ROCK || g == INDIE || g == COUNTRY || g == FOLK || g == BLUEGRASS ? 3.0L : 3.5L));
            size_t sectionNoteCount = 0;
            long double phraseDur = 4.0L * (60.0L / bpm);
            long double phraseStart = t;

            // Get chord progression
            std::vector<int> prog;
            if (chordProgressions.find(templateName) != chordProgressions.end()) {
                prog = chordProgressions[templateName];
            } else {
                auto progs = getChordProgressions(scaleName, g);
                prog = progs[static_cast<size_t>(rng.random_L() * progs.size())];
                if (templateName == "Verse" || templateName == "Chorus" || templateName == "Bridge" || templateName == "Intro" || templateName == "Outro") {
                    chordProgressions[templateName] = prog;
                }
            }
            size_t chordIdx = 0;

            while (t < sectionEnd && sectionNoteCount < maxNotes) {
                if (invalidFreqCount >= maxInvalidFreqs) {
                    ::SDL_Log("Aborting piano generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                    break;
                }
                if (rng.random_L() < restProb) {
                    t += getRandomDuration(g, section.progress, bpm);
                    t = snapToBeatGrid(t, bpm);
                    continue;
                }

                bool useChord = rng.random_L() < chordProb;
                bool useArpeggio = !useChord && rng.random_L() < arpeggioProb;
                bool useMelody = !useChord && !useArpeggio && rng.random_L() < melodyProb;

                if (useChord) {
                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                    for (size_t i = 0; i < chord.size() && sectionNoteCount < maxNotes; ++i) {
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = getRandomDuration(g, section.progress, bpm) * (rng.random_L() < 0.3L ? 2.0L : 1.0L);
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.125L;

                        long double targetFreq = chord[i];
                        while (targetFreq > 4000.0L) targetFreq /= 2.0L;
                        while (targetFreq < 27.5L) targetFreq *= 2.0L;
                        note.freq = getClosestFreq(targetFreq, pianoFreqs);

                        if (!std::isfinite(note.freq)) {
                            ::SDL_Log("Invalid piano frequency at t=%.2Lf, using 27.5 Hz", t);
                            note.freq = 27.5L;
                            invalidFreqCount++;
                        }
                        note.volume = 0.5L + 0.15L * section.progress;
                        note.velocity = piano.instrument == "piano_soft" ? 0.7L + 0.15L * rng.random_L() : 0.8L + 0.15L * rng.random_L();
                        note.phoneme = -1;
                        note.open = false;
                        piano.notes.push_back(note);
                        sectionNoteCount++;
                    }
                    t += getRandomDuration(g, section.progress, bpm);
                    t = snapToBeatGrid(t, bpm);
                    chordIdx++;
                } else if (useArpeggio) {
                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                    long double noteDur = 60.0L / (bpm * (g == CLASSICAL || g == AMBIENT ? 2.0L : 4.0L));
                    for (size_t i = 0; i < chord.size() && t < sectionEnd && sectionNoteCount < maxNotes; ++i) {
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = noteDur;
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.125L;

                        long double targetFreq = chord[i % chord.size()];
                        while (targetFreq > 4000.0L) targetFreq /= 2.0L;
                        while (targetFreq < 27.5L) targetFreq *= 2.0L;
                        note.freq = getClosestFreq(targetFreq, pianoFreqs);

                        if (!std::isfinite(note.freq)) {
                            ::SDL_Log("Invalid piano frequency at t=%.2Lf, using 27.5 Hz", t);
                            note.freq = 27.5L;
                            invalidFreqCount++;
                        }
                        note.volume = 0.45L + 0.1L * section.progress;
                        note.velocity = piano.instrument == "piano_soft" ? 0.65L + 0.15L * rng.random_L() : 0.75L + 0.15L * rng.random_L();
                        note.phoneme = -1;
                        note.open = false;
                        piano.notes.push_back(note);
                        t += note.duration;
                        sectionNoteCount++;
                    }
                    t = snapToBeatGrid(t, bpm);
                    chordIdx++;
                } else if (useMelody) {
                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = getRandomDuration(g, section.progress, bpm) * (rng.random_L() < 0.4L ? 1.5L : 1.0L);
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.125L;

                    int step = static_cast<int>(rng.random_L() * 3) - 1;
                    currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                    long double targetFreq = rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L);
                    while (targetFreq > 4000.0L) targetFreq /= 2.0L;
                    while (targetFreq < 27.5L) targetFreq *= 2.0L;
                    currentFreq = note.freq = getClosestFreq(targetFreq, pianoFreqs);

                    if (!std::isfinite(note.freq)) {
                        ::SDL_Log("Invalid piano frequency at t=%.2Lf, using 27.5 Hz", t);
                        note.freq = 27.5L;
                        invalidFreqCount++;
                    }
                    note.volume = 0.5L + 0.1L * section.progress;
                    note.velocity = piano.instrument == "piano_soft" ? 0.7L + 0.15L * rng.random_L() : 0.85L + 0.15L * rng.random_L();
                    note.phoneme = -1;
                    note.open = false;
                    piano.notes.push_back(note);
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);
                    sectionNoteCount++;
                } else {
                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = getRandomDuration(g, section.progress, bpm);
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.125L;

                    int step = static_cast<int>(rng.random_L() * 3) - 1;
                    currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                    long double targetFreq = rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L);
                    while (targetFreq > 4000.0L) targetFreq /= 2.0L;
                    while (targetFreq < 27.5L) targetFreq *= 2.0L;
                    currentFreq = note.freq = getClosestFreq(targetFreq, pianoFreqs);

                    if (!std::isfinite(note.freq)) {
                        ::SDL_Log("Invalid piano frequency at t=%.2Lf, using 27.5 Hz", t);
                        note.freq = 27.5L;
                        invalidFreqCount++;
                    }
                    note.volume = 0.45L + 0.1L * section.progress;
                    note.velocity = piano.instrument == "piano_soft" ? 0.65L + 0.15L * rng.random_L() : 0.8L + 0.15L * rng.random_L();
                    note.phoneme = -1;
                    note.open = false;
                    piano.notes.push_back(note);
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);
                    sectionNoteCount++;
                }

                if (t >= phraseStart + phraseDur) {
                    phraseStart = t;
                    if (rng.random_L() < 0.3L) {
                        t += 60.0L / bpm;
                        t = snapToBeatGrid(t, bpm);
                    }
                    chordIdx++;
                }
            }
            ::SDL_Log("Generated %zu notes for piano in section %s", sectionNoteCount, section.name.c_str());

            // Store template
            if (templateName == "Verse" || templateName == "Chorus" || templateName == "Bridge" || templateName == "Intro" || templateName == "Outro") {
                Part templatePart = piano;
                templatePart.notes.clear();
                templatePart.panAutomation.clear();
                templatePart.volumeAutomation.clear();
                templatePart.reverbMixAutomation.clear();
                for (const auto& note : piano.notes) {
                    if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                        Note templateNote = note;
                        templateNote.startTime -= section.startTime;
                        templatePart.notes.push_back(templateNote);
                    }
                }
                for (const auto& [time, value] : piano.panAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.panAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : piano.volumeAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : piano.reverbMixAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                sectionTemplates[templateName + "_Piano_" + piano.instrument] = templatePart;
                ::SDL_Log("Stored piano template %s_%s with %zu notes", templateName.c_str(), piano.instrument.c_str(), templatePart.notes.size());
            }
        }
        ::SDL_Log("Generated piano with total %zu notes, %zu invalid frequencies encountered", piano.notes.size(), invalidFreqCount);
        return piano;
    }

    Part generateGuitar(Genre g, const std::string& scaleName, long double rootFreq, long double totalDur, const std::vector<Section>& sections, long double bpm) {
        static thread_local AudioUtils::RandomGenerator rng;
        ::SDL_Log("Generating guitar for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
        
        Part guitar;
        guitar.instrument = (rng.random_L() < 0.2L) ? "guitar_bright" :
                            (rng.random_L() < 0.4L) ? "guitar_clean" : "guitar";
        
        // Set initial parameters
        guitar.pan = (rng.random_L() < 0.5L) ? 0.2L : -0.2L;
        guitar.reverbMix = (g == AMBIENT || g == NEW_AGE || g == CLASSICAL) ? 0.4L :
                           (g == ROCK || g == INDIE || g == BLUES || g == COUNTRY || g == FOLK || g == BLUEGRASS) ? 0.3L :
                           (g == JAZZ || g == CLASSICAL_JAZZ_FUSION || g == LATIN || g == REGGAE || g == WORLD) ? 0.25L : 0.2L;
        guitar.sectionName = "Guitar";
        guitar.useReverb = (g == AMBIENT || g == NEW_AGE || g == CLASSICAL || g == ROCK || g == INDIE || g == BLUES ||
                            g == COUNTRY || g == FOLK || g == BLUEGRASS || g == JAZZ || g == CLASSICAL_JAZZ_FUSION ||
                            g == LATIN || g == REGGAE || g == WORLD || rng.random_L() < 0.5L);
        guitar.reverbDelay = 0.4L;
        guitar.reverbDecay = 0.7L;
        guitar.useDistortion = (g == ROCK || g == BLUES || g == INDIE || g == FUNK || g == METAL || rng.random_L() < 0.3L);
        guitar.distortionDrive = guitar.instrument == "guitar_bright" ? 1.8L :
                                 guitar.instrument == "guitar_clean" ? 1.4L : 1.6L;
        guitar.distortionThreshold = 0.9L;

        // Probabilities for note types
        const long double restProb = (g == AMBIENT || g == NEW_AGE || g == CLASSICAL) ? 0.4L :
                                     (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION) ? 0.35L :
                                     (g == ROCK || g == INDIE || g == COUNTRY || g == FOLK || g == BLUEGRASS) ? 0.25L :
                                     (g == FUNK || g == RNB || g == SOUL || g == GOSPEL) ? 0.3L :
                                     (g == LATIN || g == REGGAE || g == REGGAETON || g == WORLD) ? 0.28L : 0.3L;
        const long double chordProb = (g == ROCK || g == COUNTRY || g == FOLK || g == BLUEGRASS || g == POP || g == INDIE) ? 0.6L :
                                     (g == JAZZ || g == CLASSICAL_JAZZ_FUSION || g == BLUES) ? 0.4L :
                                     (g == FUNK || g == RNB || g == SOUL || g == GOSPEL) ? 0.5L :
                                     (g == AMBIENT || g == NEW_AGE || g == CLASSICAL) ? 0.3L : 0.45L;
        const long double arpeggioProb = (g == CLASSICAL || g == AMBIENT || g == NEW_AGE || g == FOLK || g == BLUEGRASS) ? 0.5L :
                                        (g == JAZZ || g == CLASSICAL_JAZZ_FUSION || g == LATIN || g == WORLD) ? 0.3L :
                                        (g == POP || g == INDIE || g == ROCK) ? 0.25L : 0.2L;
        const long double melodyProb = (g == JAZZ || g == BLUES || g == CLASSICAL_JAZZ_FUSION || g == RNB || g == SOUL || g == GOSPEL) ? 0.4L :
                                      (g == ROCK || g == INDIE || g == COUNTRY || g == FOLK || g == BLUEGRASS) ? 0.35L :
                                      (g == CLASSICAL || g == AMBIENT || g == NEW_AGE || g == LATIN || g == REGGAE || g == WORLD) ? 0.3L : 0.25L;

        // Reserve space
        guitar.notes.reserve(500);
        guitar.panAutomation.reserve(36);
        guitar.volumeAutomation.reserve(36);
        guitar.reverbMixAutomation.reserve(36);

        size_t invalidFreqCount = 0;
        const size_t maxInvalidFreqs = 100;

        // Automation
        for (const auto& section : sections) {
            long double t = section.startTime;
            long double end = section.endTime;
            long double step = (end - t) / 4.0L;
            for (int i = 0; i < 4 && t < end; ++i) {
                long double pan = std::clamp(guitar.pan + 0.1L * (rng.random_L() - 0.5L), -1.0L, 1.0L);
                long double vol = std::clamp(0.45L + 0.15L * rng.random_L(), 0.45L, 1.0L);
                long double rev = std::clamp(guitar.reverbMix + 0.1L * rng.random_L(), 0.0L, 1.0L);
                guitar.panAutomation.emplace_back(t, pan);
                guitar.volumeAutomation.emplace_back(t, vol);
                guitar.reverbMixAutomation.emplace_back(t, rev);
                t += step;
            }
        }

        const auto& intervals = scales.at(scaleName);
        size_t currentIdx = static_cast<size_t>(rng.random_L() * intervals.size());
        long double currentFreq = getClosestFreq(rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L), guitarFreqs);

        for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
            const auto& section = sections[sectionIdx];
            std::string templateName = section.templateName;

            // Reuse templates
            if (sectionTemplates.find(templateName + "_Guitar_" + guitar.instrument) != sectionTemplates.end() &&
                (templateName == "Verse" || templateName == "Chorus" || templateName == "Bridge" || templateName == "Intro" || templateName == "Outro")) {
                long double intensity = (section.name.find("Chorus") != std::string::npos || section.name.find("Bridge") != std::string::npos ||
                                         section.name.find("2") != std::string::npos) ? 1.15L : 1.0L;
                bool transpose = (section.name.find("Chorus2") != std::string::npos && rng.random_L() < 0.3L);
                long double transposeSemitones = transpose ? 2.0L : 0.0L;
                Part varied = varyPart(sectionTemplates[templateName + "_Guitar_" + guitar.instrument], section.startTime, g, intensity, transpose, transposeSemitones, scaleName);
                guitar.notes.insert(guitar.notes.end(), varied.notes.begin(), varied.notes.end());
                guitar.panAutomation.insert(guitar.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                guitar.volumeAutomation.insert(guitar.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                guitar.reverbMixAutomation.insert(guitar.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                ::SDL_Log("Reused guitar template %s_%s for section %s with %zu notes", templateName.c_str(), guitar.instrument.c_str(), section.name.c_str(), varied.notes.size());
                continue;
            }

            long double t = section.startTime;
            long double sectionEnd = section.endTime;
            long double sectionDur = sectionEnd - t;
            size_t maxNotes = static_cast<size_t>(sectionDur * (
                g == ROCK || g == INDIE || g == BLUES || g == FUNK || g == METAL ? 3.5L :
                g == AMBIENT || g == NEW_AGE || g == CLASSICAL ? 2.0L :
                g == JAZZ || g == CLASSICAL_JAZZ_FUSION || g == LATIN || g == REGGAE || g == WORLD ? 3.0L : 2.5L));
            size_t sectionNoteCount = 0;
            long double phraseDur = 4.0L * (60.0L / bpm);
            long double phraseStart = t;

            // Get chord progression
            std::vector<int> prog;
            if (chordProgressions.find(templateName) != chordProgressions.end()) {
                prog = chordProgressions[templateName];
            } else {
                auto progs = getChordProgressions(scaleName, g);
                prog = progs[static_cast<size_t>(rng.random_L() * progs.size())];
                if (templateName == "Verse" || templateName == "Chorus" || templateName == "Bridge" || templateName == "Intro" || templateName == "Outro") {
                    chordProgressions[templateName] = prog;
                }
            }
            size_t chordIdx = 0;

            while (t < sectionEnd && sectionNoteCount < maxNotes) {
                if (invalidFreqCount >= maxInvalidFreqs) {
                    ::SDL_Log("Aborting guitar generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                    break;
                }
                if (rng.random_L() < restProb) {
                    t += getRandomDuration(g, section.progress, bpm);
                    t = snapToBeatGrid(t, bpm);
                    continue;
                }

                bool useChord = rng.random_L() < chordProb;
                bool useArpeggio = !useChord && rng.random_L() < arpeggioProb;
                bool useMelody = !useChord && !useArpeggio && rng.random_L() < melodyProb;

                if (useChord) {
                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                    for (size_t i = 0; i < chord.size() && sectionNoteCount < maxNotes; ++i) {
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = getRandomDuration(g, section.progress, bpm) * (rng.random_L() < 0.3L ? 2.0L : 1.0L);
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.125L;

                        long double targetFreq = chord[i];
                        while (targetFreq > 1300.0L) targetFreq /= 2.0L;
                        while (targetFreq < 82.0L) targetFreq *= 2.0L;
                        note.freq = getClosestFreq(targetFreq, guitarFreqs);

                        if (!std::isfinite(note.freq)) {
                            ::SDL_Log("Invalid guitar frequency at t=%.2Lf, using 82.0 Hz", t);
                            note.freq = 82.0L;
                            invalidFreqCount++;
                        }
                        note.volume = 0.5L + 0.1L * section.progress;
                        note.velocity = guitar.instrument == "guitar_clean" ? 0.7L + 0.15L * rng.random_L() : 0.8L + 0.15L * rng.random_L();
                        note.phoneme = -1;
                        note.open = false;
                        guitar.notes.push_back(note);
                        sectionNoteCount++;
                    }
                    t += getRandomDuration(g, section.progress, bpm);
                    t = snapToBeatGrid(t, bpm);
                    chordIdx++;
                } else if (useArpeggio) {
                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                    long double noteDur = 60.0L / (bpm * (g == CLASSICAL || g == AMBIENT || g == NEW_AGE ? 2.0L : 4.0L));
                    for (size_t i = 0; i < chord.size() && t < sectionEnd && sectionNoteCount < maxNotes; ++i) {
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = noteDur;
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.125L;

                        long double targetFreq = chord[i % chord.size()];
                        while (targetFreq > 1300.0L) targetFreq /= 2.0L;
                        while (targetFreq < 82.0L) targetFreq *= 2.0L;
                        note.freq = getClosestFreq(targetFreq, guitarFreqs);

                        if (!std::isfinite(note.freq)) {
                            ::SDL_Log("Invalid guitar frequency at t=%.2Lf, using 82.0 Hz", t);
                            note.freq = 82.0L;
                            invalidFreqCount++;
                        }
                        note.volume = 0.45L + 0.1L * section.progress;
                        note.velocity = guitar.instrument == "guitar_clean" ? 0.65L + 0.15L * rng.random_L() : 0.75L + 0.15L * rng.random_L();
                        note.phoneme = -1;
                        note.open = false;
                        guitar.notes.push_back(note);
                        t += note.duration;
                        sectionNoteCount++;
                    }
                    t = snapToBeatGrid(t, bpm);
                    chordIdx++;
                } else if (useMelody) {
                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = getRandomDuration(g, section.progress, bpm) * (rng.random_L() < 0.4L ? 1.5L : 1.0L);
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.125L;

                    int step = static_cast<int>(rng.random_L() * 3) - 1;
                    currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                    long double targetFreq = rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L);
                    while (targetFreq > 1300.0L) targetFreq /= 2.0L;
                    while (targetFreq < 82.0L) targetFreq *= 2.0L;
                    currentFreq = note.freq = getClosestFreq(targetFreq, guitarFreqs);

                    if (!std::isfinite(note.freq)) {
                        ::SDL_Log("Invalid guitar frequency at t=%.2Lf, using 82.0 Hz", t);
                        note.freq = 82.0L;
                        invalidFreqCount++;
                    }
                    note.volume = 0.5L + 0.1L * section.progress;
                    note.velocity = guitar.instrument == "guitar_clean" ? 0.7L + 0.15L * rng.random_L() : 0.85L + 0.15L * rng.random_L();
                    note.phoneme = -1;
                    note.open = false;
                    guitar.notes.push_back(note);
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);
                    sectionNoteCount++;
                } else {
                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = getRandomDuration(g, section.progress, bpm);
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0L) note.duration = 0.125L;

                    int step = static_cast<int>(rng.random_L() * 3) - 1;
                    currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                    long double targetFreq = rootFreq * std::pow(2.0L, intervals[currentIdx] / 12.0L);
                    while (targetFreq > 1300.0L) targetFreq /= 2.0L;
                    while (targetFreq < 82.0L) targetFreq *= 2.0L;
                    currentFreq = note.freq = getClosestFreq(targetFreq, guitarFreqs);

                    if (!std::isfinite(note.freq)) {
                        ::SDL_Log("Invalid guitar frequency at t=%.2Lf, using 82.0 Hz", t);
                        note.freq = 82.0L;
                        invalidFreqCount++;
                    }
                    note.volume = 0.45L + 0.1L * section.progress;
                    note.velocity = guitar.instrument == "guitar_clean" ? 0.65L + 0.15L * rng.random_L() : 0.8L + 0.15L * rng.random_L();
                    note.phoneme = -1;
                    note.open = false;
                    guitar.notes.push_back(note);
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);
                    sectionNoteCount++;
                }

                if (t >= phraseStart + phraseDur) {
                    phraseStart = t;
                    if (rng.random_L() < 0.4L) {
                        t += 60.0L / bpm;
                        t = snapToBeatGrid(t, bpm);
                    }
                    chordIdx++;
                }
            }
            ::SDL_Log("Generated %zu notes for guitar in section %s", sectionNoteCount, section.name.c_str());

            // Store template
            if (templateName == "Verse" || templateName == "Chorus" || templateName == "Bridge" || templateName == "Intro" || templateName == "Outro") {
                Part templatePart = guitar;
                templatePart.notes.clear();
                templatePart.panAutomation.clear();
                templatePart.volumeAutomation.clear();
                templatePart.reverbMixAutomation.clear();
                for (const auto& note : guitar.notes) {
                    if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                        Note templateNote = note;
                        templateNote.startTime -= section.startTime;
                        templatePart.notes.push_back(templateNote);
                    }
                }
                for (const auto& [time, value] : guitar.panAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.panAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : guitar.volumeAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : guitar.reverbMixAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                sectionTemplates[templateName + "_Guitar_" + guitar.instrument] = templatePart;
                ::SDL_Log("Stored guitar template %s_%s with %zu notes", templateName.c_str(), guitar.instrument.c_str(), templatePart.notes.size());
            }
        }
        ::SDL_Log("Generated guitar with total %zu notes, %zu invalid frequencies encountered", guitar.notes.size(), invalidFreqCount);
        return guitar;
    }

Part generateBass(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
    ::SDL_Log("Generating bass for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
    Part bass;
    bass.instrument = (g == ROCK || g == PUNK || g == METAL || g == FUNK || g == BLUES) ? "bass" : 
                      (g == TECHNO || g == EDM || g == AMBIENT) ? "subbass" : "bass";
    bass.pan = 0.0f; // Centered for low-end presence
    bass.reverbMix = (g == AMBIENT || g == TECHNO || g == EDM) ? 0.25f : 0.15f;
    bass.sectionName = "Bass";
    bass.useReverb = (g == AMBIENT || g == TECHNO || g == EDM || rng() % 2);
    bass.reverbDelay = 0.15f;
    bass.reverbDecay = 0.5f;
    bass.useDistortion = (g == ROCK || g == METAL || g == PUNK || rng() % 4 == 0);
    bass.distortionDrive = 1.5f;
    bass.distortionThreshold = 0.8f;

    const float restProb = (g == JAZZ || g == BLUES) ? 0.4f : 0.3f;
    const float walkingProb = (g == JAZZ || g == BLUES) ? 0.65f : (g == FUNK) ? 0.5f : 0.15f;
    bass.notes.reserve(250);
    bass.panAutomation.reserve(36);
    bass.volumeAutomation.reserve(36);
    bass.reverbMixAutomation.reserve(36);

    size_t invalidFreqCount = 0;
    const size_t maxInvalidFreqs = 100;

    // Define bass-specific frequency pool (40–200 Hz, covering E1 to ~G3)
    const std::vector<float> bassFreqs = {
        41.20f, 43.65f, 46.25f, 49.00f, 51.91f, 55.00f, 58.27f, 61.74f, // E1 to B1
        65.41f, 69.30f, 73.42f, 77.78f, 82.41f, 87.31f, 92.50f, 98.00f, // C2 to B2
        103.83f, 110.00f, 116.54f, 123.47f, 130.81f, 138.59f, 146.83f, 155.56f, // C3 to G3
        164.81f, 174.61f, 185.00f, 196.00f // G#3 to B3
    };

    // Automation for smooth transitions
    for (const auto& section : sections) {
        float t = section.startTime;
        float end = section.endTime;
        float step = (end - t) / 4.0f;
        for (int i = 0; i < 4 && t < end; ++i) {
            float pan = std::max(-1.0f, std::min(1.0f, bass.pan + (rng() % 5 - 2.5f) / 100.0f));
            float vol = std::max(0.45f, std::min(1.0f, 0.45f + (rng() % 10) / 100.0f));
            float rev = std::max(0.0f, std::min(1.0f, bass.reverbMix + (rng() % 5) / 100.0f));
            bass.panAutomation.emplace_back(t, pan);
            bass.volumeAutomation.emplace_back(t, vol);
            bass.reverbMixAutomation.emplace_back(t, rev);
            t += step;
        }
    }

    for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
        const auto& section = sections[sectionIdx];
        std::string templateName = section.templateName;

        // Reuse templates for Verse/Chorus
        if (sectionTemplates.find(templateName + "_Bass") != sectionTemplates.end() &&
            (templateName == "Verse" || templateName == "Chorus")) {
            long double intensity = (section.name == "Chorus2" || section.name == "Verse2") ? 1.15L : 1.0L;
            Part varied = varyPart(sectionTemplates[templateName + "_Bass"], section.startTime, intensity);
            bass.notes.insert(bass.notes.end(), varied.notes.begin(), varied.notes.end());
            bass.panAutomation.insert(bass.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
            bass.volumeAutomation.insert(bass.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
            bass.reverbMixAutomation.insert(bass.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
            ::SDL_Log("Reused bass template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
            continue;
        }

        float t = section.startTime;
        float sectionEnd = section.endTime;
        float sectionDur = sectionEnd - t;
        size_t maxNotes = static_cast<size_t>(sectionDur * (g == FUNK || g == JAZZ || g == BLUES ? 4.0f : 2.0f));
        size_t sectionNoteCount = 0;
        bool useWalking = (rng() / static_cast<float>(rng.max()) < walkingProb);

        // Get chord progression
        std::vector<int> prog;
        if (chordProgressions.find(templateName) != chordProgressions.end()) {
            prog = chordProgressions[templateName];
        } else {
            auto progs = getChordProgressions(scaleName, g);
            prog = progs[rng() % progs.size()];
            if (templateName == "Verse" || templateName == "Chorus") {
                chordProgressions[templateName] = prog;
            }
        }
        size_t chordIdx = 0;

        while (t < sectionEnd && sectionNoteCount < maxNotes) {
            if (invalidFreqCount >= maxInvalidFreqs) {
                ::SDL_Log("Aborting bass generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                break;
            }
            if (rng() / static_cast<float>(rng.max()) < restProb && !useWalking) {
                t += getRandomDuration(g, section.progress, bpm);
                t = snapToBeatGrid(t, bpm);
                continue;
            }

            Note note;
            note.startTime = snapToBeatGrid(t, bpm);
            note.duration = useWalking ? 60.0f / (bpm * 4) : getRandomDuration(g, section.progress, bpm);
            if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
            if (!std::isfinite(note.duration) || note.duration <= 0.0f) {
                note.duration = 0.0625f;
            }

            // Select frequency
            if (useWalking) {
                // Walking bass: step-wise motion within scale
                const auto& intervals = scales.at(scaleName);
                auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                float rootNote = chord[0];
                size_t currentIdx = 0;
                for (size_t j = 0; j < intervals.size(); ++j) {
                    float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                    if (std::abs(rootNote - freq) < 1e-3) {
                        currentIdx = j;
                        break;
                    }
                }
                int step = (rng() % 3) - 1; // -1, 0, or 1 for smooth transitions
                currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                float targetFreq = rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f);
                // Ensure frequency is in bass range
                while (targetFreq > 200.0f) targetFreq /= 2.0f;
                while (targetFreq < 40.0f) targetFreq *= 2.0f;
                note.freq = getClosestFreq(targetFreq, bassFreqs);
            } else {
                // Root note or chord tone, biased toward root
                auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                float targetFreq = chord[rng() % chord.size()];
                // Prefer root note 70% of the time
                if (rng() / static_cast<float>(rng.max()) < 0.7f) targetFreq = chord[0];
                // Ensure frequency is in bass range
                while (targetFreq > 200.0f) targetFreq /= 2.0f;
                while (targetFreq < 40.0f) targetFreq *= 2.0f;
                note.freq = getClosestFreq(targetFreq, bassFreqs);
                // Emphasize downbeats in ROCK/EDM
                if ((g == ROCK || g == EDM || g == METAL) && std::fmod(note.startTime, 4 * 60.0f / bpm) < 0.1f) {
                    note.velocity = 0.95f;
                    note.volume = 0.55f;
                }
            }

            if (!std::isfinite(note.freq)) {
                ::SDL_Log("Invalid bass frequency at t=%.2f, using 41.20 Hz", t);
                note.freq = 41.20f; // Default to E1
                invalidFreqCount++;
            }
            note.volume = (g == ROCK || g == METAL || g == EDM) ? 0.5f : 0.45f + 0.1f * section.progress;
            note.velocity = 0.85f + 0.15f * (rng() % 100) / 100.0f;
            note.phoneme = -1;
            note.open = false;
            bass.notes.push_back(note);
            t += note.duration;
            t = snapToBeatGrid(t, bpm);
            sectionNoteCount++;
            if (!useWalking && note.duration >= 0.25f) chordIdx++;
        }
        ::SDL_Log("Generated %zu notes for bass in section %s", sectionNoteCount, section.name.c_str());

        // Store template for Verse/Chorus
        if (templateName == "Verse" || templateName == "Chorus") {
            Part templatePart = bass;
            templatePart.notes.clear();
            templatePart.panAutomation.clear();
            templatePart.volumeAutomation.clear();
            templatePart.reverbMixAutomation.clear();
            for (const auto& note : bass.notes) {
                if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                    Note templateNote = note;
                    templateNote.startTime -= section.startTime;
                    templatePart.notes.push_back(templateNote);
                }
            }
            for (const auto& [time, value] : bass.panAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.panAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : bass.volumeAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : bass.reverbMixAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                }
            }
            sectionTemplates[templateName + "_Bass"] = templatePart;
            ::SDL_Log("Stored bass template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
        }
    }
    ::SDL_Log("Generated bass with total %zu notes, %zu invalid frequencies encountered", bass.notes.size(), invalidFreqCount);
    return bass;
}

        Part generateArpeggio(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
            Part arp;
            arp.instrument = (g == CLASSICAL) ? "piano" : (g == EDM || g == TECHNO) ? "syntharp" : "leadsynth";
            arp.pan = (rng() % 2) ? 0.4f : -0.4f;
            arp.reverbMix = (g == AMBIENT || g == EDM || g == TECHNO) ? 0.5f : 0.3f;
            arp.sectionName = "Arpeggio";
            arp.useReverb = true;
            arp.reverbDelay = 0.1f;
            arp.reverbDecay = 0.6f;
            arp.reverbMixFactor = arp.reverbMix;
            arp.useDistortion = (g == EDM || g == TECHNO || rng() % 4 == 0);
            arp.distortionDrive = 1.3f;
            arp.distortionThreshold = 0.8f;

            const float restProb = 0.3f;
            arp.notes.reserve(500);
            arp.panAutomation.reserve(36);
            arp.volumeAutomation.reserve(36);
            arp.reverbMixAutomation.reserve(36);

            size_t invalidFreqCount = 0;
            const size_t maxInvalidFreqs = 100;

            for (const auto& section : sections) {
                float t = section.startTime;
                float end = section.endTime;
                float step = (end - t) / 4.0f;
                for (int i = 0; i < 4 && t < end; ++i) {
                    float pan = std::max(-1.0f, std::min(1.0f, arp.pan + (rng() % 10 - 5) / 100.0f));
                    float vol = std::max(0.3f, std::min(1.0f, 0.3f + (rng() % 10) / 100.0f));
                    float rev = std::max(0.0f, std::min(1.0f, arp.reverbMix + (rng() % 10) / 100.0f));
                    arp.panAutomation.emplace_back(t, pan);
                    arp.volumeAutomation.emplace_back(t, vol);
                    arp.reverbMixAutomation.emplace_back(t, rev);
                    t += step;
                }
            }

            for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
                const auto& section = sections[sectionIdx];
                std::string templateName = section.templateName;

                if (sectionTemplates.find(templateName + "_Arpeggio") != sectionTemplates.end() &&
                    (templateName == "Verse" || templateName == "Chorus")) {
                    float intensity = (section.name == "Chorus2" || section.name == "Verse2") ? 1.05f : 1.0f;
                    Part varied = varyPart(sectionTemplates[templateName + "_Arpeggio"], section.startTime, intensity);
                    arp.notes.insert(arp.notes.end(), varied.notes.begin(), varied.notes.end());
                    arp.panAutomation.insert(arp.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                    arp.volumeAutomation.insert(arp.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                    arp.reverbMixAutomation.insert(arp.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                    ::SDL_Log("Reused arpeggio template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
                    continue;
                }

                float t = section.startTime;
                float sectionEnd = section.endTime;
                size_t maxNotes = static_cast<size_t>((sectionEnd - t) * 4.0f);
                size_t sectionNoteCount = 0;

                std::vector<int> prog;
                if (chordProgressions.find(templateName) != chordProgressions.end()) {
                    prog = chordProgressions[templateName];
                } else {
                    auto progs = getChordProgressions(scaleName, g);
                    prog = progs[rng() % progs.size()];
                    if (templateName == "Verse" || templateName == "Chorus") {
                        chordProgressions[templateName] = prog;
                    }
                }

                float arpDur = (g == EDM || g == TECHNO) ? 60.0f / (bpm * 4) : 60.0f / (bpm * 2);
                size_t chordIdx = 0;

                while (t < sectionEnd && sectionNoteCount < maxNotes) {
                    if (invalidFreqCount >= maxInvalidFreqs) {
                        ::SDL_Log("Aborting arpeggio generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                        break;
                    }
                    if (rng() / static_cast<float>(rng.max()) < restProb) {
                        t += arpDur;
                        t = snapToBeatGrid(t, bpm);
                        continue;
                    }

                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, rng() % 2);
                    if (chord.empty() || !std::all_of(chord.begin(), chord.end(), [](float f) { return std::isfinite(f); })) {
                        ::SDL_Log("Invalid chord frequencies in arpeggio, skipping");
                        t += arpDur;
                        invalidFreqCount++;
                        continue;
                    }

                    std::vector<size_t> order(chord.size());
                    std::iota(order.begin(), order.end(), 0);
                    if (g != CLASSICAL) std::shuffle(order.begin(), order.end(), rng);

                    for (size_t i : order) {
                        if (t >= sectionEnd || sectionNoteCount >= maxNotes) break;
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = arpDur;
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0f) {
                            note.duration = 0.0625f;
                        }
                        note.freq = chord[i];
                        note.volume = 0.3f + 0.1f * section.progress;
                        note.velocity = 0.7f + 0.2f * (rng() % 100) / 100.0f;
                        note.phoneme = -1;
                        note.open = false;
                        arp.notes.push_back(note);
                        t += note.duration;
                        sectionNoteCount++;
                    }
                    chordIdx++;
                    t = snapToBeatGrid(t, bpm);
                }
                ::SDL_Log("Generated %zu notes for arpeggio in section %s", sectionNoteCount, section.name.c_str());

                if (templateName == "Verse" || templateName == "Chorus") {
                    Part templatePart = arp;
                    templatePart.notes.clear();
                    templatePart.panAutomation.clear();
                    templatePart.volumeAutomation.clear();
                    templatePart.reverbMixAutomation.clear();
                    for (const auto& note : arp.notes) {
                        if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                            Note templateNote = note;
                            templateNote.startTime -= section.startTime;
                            templatePart.notes.push_back(templateNote);
                        }
                    }
                    for (const auto& [time, value] : arp.panAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.panAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : arp.volumeAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : arp.reverbMixAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    sectionTemplates[templateName + "_Arpeggio"] = templatePart;
                    ::SDL_Log("Stored arpeggio template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
                }
            }
            ::SDL_Log("Generated arpeggio with total %zu notes, %zu invalid frequencies encountered", arp.notes.size(), invalidFreqCount);
            return arp;
        }

        Part generateHarmony(Genre g, const std::string& scaleName, float rootFreq, float totalDur,
                             const std::vector<Section>& sections, float bpm) {
            ::SDL_Log("Generating harmony for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
            Part harmony;
            harmony.sectionName = "Harmony";
            harmony.instrument = (g == CLASSICAL) ? "strings" : (g == AMBIENT) ? "pad" : "organ";
            harmony.pan = 0.0f;
            harmony.reverbMix = (g == AMBIENT || g == CLASSICAL) ? 0.5f : 0.2f;
            harmony.useReverb = (g == AMBIENT || g == CLASSICAL || g == GOSPEL);
            harmony.reverbDelay = 0.1f;
            harmony.reverbDecay = 0.9f;
            harmony.reverbMixFactor = 0.5f;
            harmony.useDistortion = (g == ROCK && rng() % 2);
            harmony.distortionDrive = 2.0f;
            harmony.distortionThreshold = 0.3f;

            harmony.notes.reserve(1000);
            harmony.panAutomation.reserve(200);
            harmony.volumeAutomation.reserve(200);
            harmony.reverbMixAutomation.reserve(200);

            size_t invalidFreqCount = 0;
            const size_t maxInvalidFreqs = 100;
            float beat = 60.0f / bpm;

            for (const auto& section : sections) {
                std::string templateName = section.templateName;
                auto it = sectionTemplates.find(templateName + "_Harmony");
                if (it != sectionTemplates.end() && (templateName == "Verse" || templateName == "Chorus")) {
                    ::SDL_Log("Using template %s for section %s", (templateName + "_Harmony").c_str(), section.name.c_str());
                    Part varied = varyPart(it->second, section.startTime, 1.0L);
                    harmony.notes.insert(harmony.notes.end(), varied.notes.begin(), varied.notes.end());
                    harmony.panAutomation.insert(harmony.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                    harmony.volumeAutomation.insert(harmony.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                    harmony.reverbMixAutomation.insert(harmony.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                    continue;
                }

                float t = section.startTime;
                float sectionDur = section.endTime - section.startTime;
                size_t sectionNoteCount = 0;
                size_t maxNotes = static_cast<size_t>(sectionDur * 0.5f);
                maxNotes = std::min(maxNotes, harmony.notes.capacity() - harmony.notes.size());

                std::vector<int> prog;
                if (chordProgressions.find(templateName) != chordProgressions.end()) {
                    prog = chordProgressions[templateName];
                } else {
                    auto progs = getChordProgressions(scaleName, g);
                    prog = progs[rng() % progs.size()];
                    if (templateName == "Verse" || templateName == "Chorus") {
                        chordProgressions[templateName] = prog;
                    }
                }

                float chordDur = beat * 4.0f;
                if (g == CLASSICAL || g == AMBIENT) chordDur *= 2.0f;
                size_t chordIdx = 0;

                while (t < section.endTime && sectionNoteCount < maxNotes && harmony.notes.size() < harmony.notes.capacity()) {
                    if (invalidFreqCount >= maxInvalidFreqs) {
                        ::SDL_Log("Aborting harmony generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                        break;
                    }
                    int degree = prog[chordIdx % prog.size()];
                    int inversion = chordIdx % 3; // Cycle through inversions for variety
                    std::vector<float> chord = buildChord(degree, scaleName, rootFreq, g, inversion);

                    for (float freq : chord) {
                        if (!std::isfinite(freq)) {
                            ::SDL_Log("Invalid chord frequency %.2f at t=%.2f, skipping", freq, t);
                            invalidFreqCount++;
                            continue;
                        }
                        Note note(freq, chordDur, t);
                        note.volume = 0.4f + 0.1f * section.progress;
                        note.velocity = 0.6f + 0.2f * (rng() % 100) / 100.0f;
                        harmony.notes.push_back(note);
                        sectionNoteCount++;
                    }

                    float pan = (g == CLASSICAL || g == AMBIENT) ? 0.0f : (rng() % 2 ? -0.2f : 0.2f);
                    if (harmony.panAutomation.size() < harmony.panAutomation.capacity()) {
                        harmony.panAutomation.emplace_back(t, pan);
                    }
                    if (harmony.volumeAutomation.size() < harmony.volumeAutomation.capacity()) {
                        harmony.volumeAutomation.emplace_back(t, 0.4f + 0.1f * section.progress);
                    }
                    if (harmony.reverbMixAutomation.size() < harmony.reverbMixAutomation.capacity()) {
                        harmony.reverbMixAutomation.emplace_back(t, harmony.reverbMix);
                    }

                    t += chordDur;
                    chordIdx++;
                }

                if (templateName == "Verse" || templateName == "Chorus") {
                    Part templatePart;
                    templatePart.instrument = harmony.instrument;
                    templatePart.pan = harmony.pan;
                    templatePart.reverbMix = harmony.reverbMix;
                    templatePart.useReverb = harmony.useReverb;
                    templatePart.reverbDelay = harmony.reverbDelay;
                    templatePart.reverbDecay = harmony.reverbDecay;
                    templatePart.reverbMixFactor = harmony.reverbMixFactor;
                    templatePart.useDistortion = harmony.useDistortion;
                    templatePart.distortionDrive = harmony.distortionDrive;
                    templatePart.distortionThreshold = harmony.distortionThreshold;

                    for (const auto& note : harmony.notes) {
                        if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                            Note templateNote = note;
                            templateNote.startTime -= section.startTime;
                            templatePart.notes.push_back(templateNote);
                        }
                    }
                    for (const auto& [time, value] : harmony.panAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.panAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : harmony.volumeAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : harmony.reverbMixAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    sectionTemplates[templateName + "_Harmony"] = templatePart;
                    ::SDL_Log("Stored harmony template %s with %zu notes", (templateName + "_Harmony").c_str(), templatePart.notes.size());
                }
            }

            ::SDL_Log("Generated harmony with total %zu notes, %zu invalid frequencies encountered", harmony.notes.size(), invalidFreqCount);
            return harmony;
        }

        Part generateVocal(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
            Part vocal;
            vocal.instrument = (rng() % 2) ? "vocal_0" : "vocal_1";
            vocal.pan = (rng() % 2) ? 0.2f : -0.2f;
            vocal.reverbMix = (g == GOSPEL || g == SOUL) ? 0.4f : 0.3f;
            vocal.sectionName = "Vocal";
            vocal.useReverb = true;
            vocal.reverbDelay = 0.15f;
            vocal.reverbDecay = 0.6f;
            vocal.reverbMixFactor = vocal.reverbMix;
            vocal.useDistortion = false;

            const float restProb = (g == RAP || g == HIPHOP) ? 0.5f : 0.4f;
            const float phraseProb = (g == GOSPEL || g == SOUL) ? 0.7f : 0.5f;
            vocal.notes.reserve(300);
            vocal.panAutomation.reserve(36);
            vocal.volumeAutomation.reserve(36);
            vocal.reverbMixAutomation.reserve(36);

            size_t invalidFreqCount = 0;
            const size_t maxInvalidFreqs = 100;

            for (const auto& section : sections) {
                float t = section.startTime;
                float end = section.endTime;
                float step = (end - t) / 4.0f;
                for (int i = 0; i < 4 && t < end; ++i) {
                    float pan = std::max(-1.0f, std::min(1.0f, vocal.pan + (rng() % 10 - 5) / 100.0f));
                    float vol = std::max(0.5f, std::min(1.0f, 0.5f + (rng() % 10) / 100.0f));
                    float rev = std::max(0.0f, std::min(1.0f, vocal.reverbMix + (rng() % 10) / 100.0f));
                    vocal.panAutomation.emplace_back(t, pan);
                    vocal.volumeAutomation.emplace_back(t, vol);
                    vocal.reverbMixAutomation.emplace_back(t, rev);
                    t += step;
                }
            }

            const auto& intervals = scales.at(scaleName);
            float currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[rng() % intervals.size()] / 12.0f));

            for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
                const auto& section = sections[sectionIdx];
                std::string templateName = section.templateName;

                if (sectionTemplates.find(templateName + "_Vocal") != sectionTemplates.end() &&
                    (templateName == "Verse" || templateName == "Chorus")) {
                    float intensity = (section.name == "Chorus2" || section.name == "Verse2") ? 1.1f : 1.0f;
                    bool transpose = (section.name == "Chorus2" && rng() % 2);
                    float transposeSemitones = transpose ? 2.0f : 0.0f;
                    Part varied = varyPart(sectionTemplates[templateName + "_Vocal"], section.startTime, intensity, transpose, transposeSemitones);
                    vocal.notes.insert(vocal.notes.end(), varied.notes.begin(), varied.notes.end());
                    vocal.panAutomation.insert(vocal.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                    vocal.volumeAutomation.insert(vocal.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                    vocal.reverbMixAutomation.insert(vocal.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                    ::SDL_Log("Reused vocal template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
                    continue;
                }

                float t = section.startTime;
                float sectionEnd = section.endTime;
                size_t maxNotes = static_cast<size_t>((sectionEnd - t) * 3.0f);
                size_t sectionNoteCount = 0;
                float phraseDur = 4.0f * 60.0f / bpm;
                float phraseStart = t;

                while (t < sectionEnd && sectionNoteCount < maxNotes) {
                    if (invalidFreqCount >= maxInvalidFreqs) {
                        ::SDL_Log("Aborting vocal generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                        break;
                    }
                    if (rng() / static_cast<float>(rng.max()) < restProb) {
                        t += getRandomDuration(g, section.progress, bpm);
                        t = snapToBeatGrid(t, bpm);
                        continue;
                    }

                    bool usePhrase = rng() / static_cast<float>(rng.max()) < phraseProb;
                    int numNotes = usePhrase ? 3 + (rng() % 3) : 1;

                    for (int i = 0; i < numNotes && t < sectionEnd && sectionNoteCount < maxNotes; ++i) {
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = getRandomDuration(g, section.progress, bpm);
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0f) {
                            note.duration = 0.0625f;
                        }
                        note.volume = 0.5f + 0.1f * section.progress;
                        note.velocity = 0.8f + 0.2f * (rng() % 100) / 100.0f;
                        note.phoneme = rng() % 7;
                        note.open = false;

                        size_t currentIdx = 0;
                        for (size_t j = 0; j < intervals.size(); ++j) {
                            float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                            if (std::abs(currentFreq - freq) < 1e-3) {
                                currentIdx = j;
                                break;
                            }
                        }
                        int step = (rng() % 2) ? 1 : -1;
                        currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                        currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f));
                        note.freq = currentFreq;
                        if (!std::isfinite(note.freq)) {
                            note.freq = 440.0f;
                            invalidFreqCount++;
                        }
                        vocal.notes.push_back(note);
                        t += note.duration;
                        sectionNoteCount++;
                    }
                    t = snapToBeatGrid(t, bpm);
                    if (t >= phraseStart + phraseDur) {
                        phraseStart = t;
                        if (rng() % 2) {
                            t += 60.0f / bpm;
                            t = snapToBeatGrid(t, bpm);
                        }
                    }
                }
                ::SDL_Log("Generated %zu notes for vocal in section %s", sectionNoteCount, section.name.c_str());

                if (templateName == "Verse" || templateName == "Chorus") {
                    Part templatePart = vocal;
                    templatePart.notes.clear();
                    templatePart.panAutomation.clear();
                    templatePart.volumeAutomation.clear();
                    templatePart.reverbMixAutomation.clear();
                    for (const auto& note : vocal.notes) {
                        if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                            Note templateNote = note;
                            templateNote.startTime -= section.startTime;
                            templatePart.notes.push_back(templateNote);
                        }
                    }
                    for (const auto& [time, value] : vocal.panAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.panAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : vocal.volumeAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : vocal.reverbMixAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    sectionTemplates[templateName + "_Vocal"] = templatePart;
                    ::SDL_Log("Stored vocal template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
                }
            }
            ::SDL_Log("Generated vocal with total %zu notes, %zu invalid frequencies encountered", vocal.notes.size(), invalidFreqCount);
            return vocal;
        }
    };
}
#endif // SONGGEN_H