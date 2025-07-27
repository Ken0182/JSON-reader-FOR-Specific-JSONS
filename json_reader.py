#!/usr/bin/env python3
"""
JSON Reader System for Sound Synthesis Configuration

This system produces a filtered, clean, minimal config JSON for sound synthesis
following the specified requirements:

1. Only outputs instruments/groups from guitar.json and group.json
2. Uses moods.json and Synthesizer.json only for AI scoring and filtering (never output)
3. Applies structure.json mappings only when relevant
4. Omits empty fields and non-applicable data
5. Keeps layering calculations internal (not in output)
6. Produces properly nested configuration with top-level instrument/group names
"""

import json
import os
from typing import Dict, List, Optional, Any, Union
from copy import deepcopy


class JSONReader:
    def __init__(self):
        self.guitar_data = {}
        self.group_data = {}
        self.moods_data = {}  # Reference only - not for output
        self.synthesizer_data = {}  # Reference only - not for output
        self.structure_data = {}
        
        # Internal AI scoring data (never output)
        self.internal_layering = {}
        self.internal_scores = {}
    
    def load_json_files(self):
        """Load all JSON files"""
        try:
            with open('guitar.json', 'r') as f:
                self.guitar_data = json.load(f)
            
            with open('group.json', 'r') as f:
                self.group_data = json.load(f)
            
            with open('moods.json', 'r') as f:
                self.moods_data = json.load(f)
            
            with open('Synthesizer.json', 'r') as f:
                self.synthesizer_data = json.load(f)
            
            with open('structure.json', 'r') as f:
                self.structure_data = json.load(f)
            
            print("✓ All JSON files loaded successfully")
            
        except FileNotFoundError as e:
            print(f"✗ Error loading file: {e}")
            raise
        except json.JSONDecodeError as e:
            print(f"✗ Error parsing JSON: {e}")
            raise
    
    def clean_empty_fields(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """
        Recursively remove empty fields, null values, and empty containers
        """
        if not isinstance(data, dict):
            return data
        
        cleaned = {}
        for key, value in data.items():
            if value is None:
                continue  # Skip null values
            
            if isinstance(value, dict):
                cleaned_dict = self.clean_empty_fields(value)
                if cleaned_dict:  # Only include non-empty dicts
                    cleaned[key] = cleaned_dict
            
            elif isinstance(value, list):
                if value:  # Only include non-empty lists
                    cleaned_list = []
                    for item in value:
                        if isinstance(item, dict):
                            cleaned_item = self.clean_empty_fields(item)
                            if cleaned_item:
                                cleaned_list.append(cleaned_item)
                        elif item is not None:
                            cleaned_list.append(item)
                    if cleaned_list:
                        cleaned[key] = cleaned_list
            
            elif isinstance(value, str):
                if value.strip():  # Only include non-empty strings
                    cleaned[key] = value
            
            else:
                cleaned[key] = value  # Include numbers, booleans, etc.
        
        return cleaned
    
    def extract_guitar_instruments(self) -> Dict[str, Dict[str, Any]]:
        """
        Extract instrument configurations from guitar.json
        Only outputs actual instrument data, properly nested
        """
        instruments = {}
        
        if 'guitar_types' not in self.guitar_data:
            return instruments
        
        for guitar_type, type_data in self.guitar_data['guitar_types'].items():
            if 'groups' not in type_data:
                continue
            
            for instrument_name, instrument_data in type_data['groups'].items():
                # Use the exact instrument name as the key
                key = instrument_name.lower().replace('_', '_').replace(' ', '_')
                
                # Build clean instrument configuration
                config = {}
                
                # ADSR envelope (only if present and not empty)
                if 'envelope' in instrument_data:
                    envelope = instrument_data['envelope']
                    adsr = {}
                    for param in ['type', 'attack', 'hold', 'decay', 'sustain', 'release', 'curve']:
                        if param in envelope:
                            adsr[param] = envelope[param]
                    if adsr:
                        config['adsr'] = adsr
                
                # Guitar-specific parameters (only non-empty)
                guitar_params = {}
                
                # Strings configuration
                if 'strings' in instrument_data:
                    strings = instrument_data['strings']
                    string_config = {}
                    for param in ['material', 'num_strings', 'tuning', 'gauge', 'detune_range', 'tension']:
                        if param in strings:
                            string_config[param] = strings[param]
                    if string_config:
                        guitar_params['strings'] = string_config
                
                # Harmonics
                if 'harmonics' in instrument_data:
                    harmonics = instrument_data['harmonics']
                    harmonic_config = {}
                    for param in ['vibe_set', 'decay_rate', 'sympathetic_resonance']:
                        if param in harmonics:
                            harmonic_config[param] = harmonics[param]
                    if harmonic_config:
                        guitar_params['harmonics'] = harmonic_config
                
                # Filter
                if 'filter' in instrument_data:
                    filter_data = instrument_data['filter']
                    filter_config = {}
                    for param in ['type', 'cutoff', 'resonance', 'envelope_amount', 'slope']:
                        if param in filter_data:
                            filter_config[param] = filter_data[param]
                    if filter_config:
                        guitar_params['filter'] = filter_config
                
                # Attack noise
                if 'attack_noise' in instrument_data:
                    attack_noise = instrument_data['attack_noise']
                    if attack_noise.get('enabled'):
                        noise_config = {}
                        for param in ['intensity', 'burst_length', 'noise_type']:
                            if param in attack_noise:
                                noise_config[param] = attack_noise[param]
                        if noise_config:
                            guitar_params['attack_noise'] = noise_config
                
                # Body resonance
                if 'body_resonance' in instrument_data:
                    body_res = instrument_data['body_resonance']
                    res_config = {}
                    for param in ['ir_file', 'mix']:
                        if param in body_res:
                            res_config[param] = body_res[param]
                    if res_config:
                        guitar_params['body_resonance'] = res_config
                
                # Pick configuration
                if 'pick' in instrument_data:
                    pick = instrument_data['pick']
                    pick_config = {}
                    for param in ['stiffness', 'noiseProbability', 'noiseIntensity', 'position']:
                        if param in pick:
                            pick_config[param] = pick[param]
                    if pick_config:
                        guitar_params['pick'] = pick_config
                
                # Vibrato
                if 'vibrato' in instrument_data:
                    vibrato = instrument_data['vibrato']
                    vib_config = {}
                    for param in ['freq_range', 'vibrato_hz', 'depth_cents']:
                        if param in vibrato:
                            vib_config[param] = vibrato[param]
                    if vib_config:
                        guitar_params['vibrato'] = vib_config
                
                if guitar_params:
                    config['guitarParams'] = guitar_params
                
                # Effects (only if present)
                if 'fx' in instrument_data and instrument_data['fx']:
                    effects = []
                    for fx in instrument_data['fx']:
                        if isinstance(fx, dict) and fx:
                            effects.append(fx)
                    if effects:
                        config['effects'] = effects
                
                # Sound characteristics (only if present)
                if 'sound_characteristics' in instrument_data:
                    sound_char = instrument_data['sound_characteristics']
                    char_config = {}
                    for param in ['timbral', 'material', 'emotional', 'dynamic']:
                        if param in sound_char:
                            char_config[param] = sound_char[param]
                    if char_config:
                        config['soundCharacteristics'] = char_config
                
                # Topological metadata (only if present)
                if 'topological_metadata' in instrument_data:
                    topo = instrument_data['topological_metadata']
                    topo_config = {}
                    for param in ['damping', 'spectral_complexity', 'manifold_position']:
                        if param in topo:
                            topo_config[param] = topo[param]
                    if topo_config:
                        config['topologicalMetadata'] = topo_config
                
                # Only add instrument if it has actual configuration data
                if config:
                    instruments[key] = config
        
        return instruments
    
    def extract_group_effects(self) -> Dict[str, Dict[str, Any]]:
        """
        Extract group/effect configurations from group.json
        Only outputs actual group data, properly nested
        """
        groups = {}
        
        if 'groups' not in self.group_data:
            return groups
        
        for group_name, group_data in self.group_data['groups'].items():
            # Use the exact group name as the key
            key = group_name.lower().replace('_', '_').replace(' ', '_')
            
            # Build clean group configuration
            config = {}
            
            # Synthesis type
            if 'synthesis_type' in group_data:
                config['synthesis_type'] = group_data['synthesis_type']
            
            # Oscillator configuration
            if 'oscillator' in group_data:
                osc = group_data['oscillator']
                osc_config = {}
                for param in ['types', 'mix_ratios', 'detune', 'modulation_index', 'carrier_ratio', 
                             'harmonics', 'morph_rate', 'table_index', 'pluck_position', 'grain_density', 'grain_size']:
                    if param in osc:
                        osc_config[param] = osc[param]
                if osc_config:
                    config['oscillator'] = osc_config
            
            # Envelope
            if 'envelope' in group_data:
                envelope = group_data['envelope']
                env_config = {}
                for param in ['type', 'attack', 'hold', 'decay', 'sustain', 'release', 'delay', 'curve']:
                    if param in envelope:
                        env_config[param] = envelope[param]
                if env_config:
                    config['envelope'] = env_config
            
            # Filter
            if 'filter' in group_data:
                filter_data = group_data['filter']
                filter_config = {}
                for param in ['type', 'cutoff', 'resonance', 'envelope_amount', 'slope']:
                    if param in filter_data:
                        filter_config[param] = filter_data[param]
                if filter_config:
                    config['filter'] = filter_config
            
            # Effects (only if present)
            if 'fx' in group_data and group_data['fx']:
                effects = []
                for fx in group_data['fx']:
                    if isinstance(fx, dict) and fx:
                        effects.append(fx)
                if effects:
                    config['fx'] = effects
            
            # Sound characteristics (only if present)
            if 'sound_characteristics' in group_data:
                sound_char = group_data['sound_characteristics']
                char_config = {}
                for param in ['timbral', 'material', 'emotional', 'dynamic']:
                    if param in sound_char:
                        char_config[param] = sound_char[param]
                if char_config:
                    config['sound_characteristics'] = char_config
            
            # Topological metadata (only if present)
            if 'topological_metadata' in group_data:
                topo = group_data['topological_metadata']
                topo_config = {}
                for param in ['damping', 'spectral_complexity', 'manifold_position']:
                    if param in topo:
                        topo_config[param] = topo[param]
                if topo_config:
                    config['topological_metadata'] = topo_config
            
            # Only add group if it has actual configuration data
            if config:
                groups[key] = config
        
        return groups
    
    def apply_structure_mappings(self, config: Dict[str, Dict[str, Any]]) -> Dict[str, Dict[str, Any]]:
        """
        Apply structure.json mappings only where relevant
        Omits structure field if no mapping exists
        """
        if 'sections' not in self.structure_data:
            return config
        
        # Create mapping from group names to structure data
        structure_map = {}
        for section in self.structure_data['sections']:
            if 'group' in section:
                group_key = section['group'].lower().replace('_', '_').replace(' ', '_')
                structure_map[group_key] = {
                    'sectionName': section.get('sectionName'),
                    'attackMul': section.get('attackMul'),
                    'decayMul': section.get('decayMul'),
                    'sustainMul': section.get('sustainMul'),
                    'releaseMul': section.get('releaseMul'),
                    'useDynamicGate': section.get('useDynamicGate'),
                    'gateThreshold': section.get('gateThreshold'),
                    'gateDecaySec': section.get('gateDecaySec')
                }
        
        # Apply structure mappings only where they exist
        for key, item_config in config.items():
            if key in structure_map:
                structure_info = structure_map[key]
                # Only include non-null structure data
                structure_config = {}
                for struct_key, struct_value in structure_info.items():
                    if struct_value is not None:
                        structure_config[struct_key] = struct_value
                
                if structure_config:
                    item_config['structure'] = structure_config
        
        return config
    
    def calculate_internal_layering(self, config: Dict[str, Dict[str, Any]]):
        """
        Calculate layering stages for internal use only (NOT for output)
        Used by next program stage (WAV writer)
        """
        for key, item_config in config.items():
            # Calculate layer based on characteristics (1-6 scale)
            layer = 3  # Default middle layer
            
            # Analyze sound characteristics for layering hints
            if 'sound_characteristics' in item_config:
                char = item_config['sound_characteristics']
                
                # Background layers (1-2)
                if char.get('dynamic') in ['ambient', 'sustained'] and char.get('timbral') in ['warm', 'soft']:
                    layer = 1
                elif char.get('timbral') in ['mellow', 'calm']:
                    layer = 2
                
                # Foreground layers (5-6)
                elif char.get('dynamic') in ['percussive'] and char.get('timbral') in ['bright', 'clear']:
                    layer = 6
                elif char.get('timbral') in ['energetic', 'aggressive']:
                    layer = 5
                
                # Middle layers (3-4)
                elif char.get('dynamic') in ['punchy', 'driving']:
                    layer = 4
            
            # Store internally - NEVER output this
            self.internal_layering[key] = layer
    
    def calculate_ai_scores(self, config: Dict[str, Dict[str, Any]], user_preferences: List[str] = None):
        """
        Calculate AI scores using moods.json and Synthesizer.json for filtering
        These scores are for internal use only (NOT for output)
        """
        if user_preferences is None:
            user_preferences = []
        
        for key, item_config in config.items():
            score = 0.0
            
            # Score based on mood matching using moods.json reference data
            if 'moods' in self.moods_data:
                for mood in self.moods_data['moods']:
                    mood_name = mood.get('name', '').lower()
                    if mood_name in [p.lower() for p in user_preferences]:
                        # Check if item matches mood characteristics
                        if 'sound_characteristics' in item_config:
                            char = item_config['sound_characteristics']
                            if 'emotional' in char:
                                for emotion in char['emotional']:
                                    if isinstance(emotion, dict) and emotion.get('tag', '').lower() == mood_name:
                                        score += emotion.get('weight', 1.0)
                            
                            # Match timbral characteristics
                            if char.get('timbral', '').lower() == mood_name:
                                score += 1.0
            
            # Score based on synthesizer section matching using Synthesizer.json reference data
            if 'sections' in self.synthesizer_data:
                for section_name, section_data in self.synthesizer_data['sections'].items():
                    if section_name.lower() in [p.lower() for p in user_preferences]:
                        # Match emotional characteristics
                        if 'emotion' in section_data:
                            emotions = section_data['emotion'].lower().split(', ')
                            if 'sound_characteristics' in item_config:
                                char = item_config['sound_characteristics']
                                if char.get('timbral', '').lower() in emotions:
                                    score += 0.8
            
            # Store internally - NEVER output this
            self.internal_scores[key] = score
    
    def generate_clean_config(self, user_preferences: List[str] = None) -> Dict[str, Dict[str, Any]]:
        """
        Generate the final clean, minimal configuration
        Following all requirements for output structure
        """
        print("🔄 Generating clean configuration...")
        
        # Extract only real instrument/group data
        instruments = self.extract_guitar_instruments()
        groups = self.extract_group_effects()
        
        # Combine into single top-level structure
        config = {}
        config.update(instruments)
        config.update(groups)
        
        # Apply structure mappings only where relevant
        config = self.apply_structure_mappings(config)
        
        # Calculate internal data (not for output)
        self.calculate_internal_layering(config)
        self.calculate_ai_scores(config, user_preferences)
        
        # Clean empty fields recursively
        config = self.clean_empty_fields(config)
        
        print(f"✓ Generated configuration with {len(config)} instruments/groups")
        print(f"✓ Internal layering calculated for {len(self.internal_layering)} items")
        print(f"✓ Internal AI scores calculated for {len(self.internal_scores)} items")
        
        return config
    
    def save_config(self, config: Dict[str, Dict[str, Any]], filename: str = "clean_config.json"):
        """
        Save the clean configuration to file
        """
        try:
            with open(filename, 'w') as f:
                json.dump(config, f, indent=2, sort_keys=True)
            print(f"✓ Configuration saved to {filename}")
        except Exception as e:
            print(f"✗ Error saving configuration: {e}")
            raise
    
    def print_internal_summary(self):
        """
        Print summary of internal calculations (for debugging/info only)
        This data is never included in the output config
        """
        print("\n📊 Internal AI Data Summary (not in output):")
        print(f"   Layering assignments: {len(self.internal_layering)} items")
        print(f"   AI scores calculated: {len(self.internal_scores)} items")
        
        if self.internal_layering:
            print("\n   Layer distribution:")
            layer_counts = {}
            for item, layer in self.internal_layering.items():
                layer_counts[layer] = layer_counts.get(layer, 0) + 1
            for layer in sorted(layer_counts.keys()):
                print(f"     Layer {layer}: {layer_counts[layer]} items")
        
        if self.internal_scores:
            print(f"\n   Score range: {min(self.internal_scores.values()):.2f} - {max(self.internal_scores.values()):.2f}")


def main():
    """
    Main function to demonstrate the JSON reader system
    """
    print("🎵 JSON Reader System for Sound Synthesis")
    print("=" * 50)
    
    # Initialize reader
    reader = JSONReader()
    
    # Load JSON files
    print("\n📁 Loading JSON files...")
    reader.load_json_files()
    
    # Generate clean configuration
    print("\n⚙️  Processing configuration...")
    
    # Example user preferences for AI scoring (optional)
    user_preferences = ["energetic", "warm", "chorus", "intro"]
    
    # Generate the clean config
    clean_config = reader.generate_clean_config(user_preferences)
    
    # Save the configuration
    print("\n💾 Saving configuration...")
    reader.save_config(clean_config)
    
    # Print internal summary
    reader.print_internal_summary()
    
    print(f"\n✅ Process complete!")
    print(f"   Output: clean_config.json")
    print(f"   Structure: Top-level instrument/group names with nested properties")
    print(f"   Clean: No empty fields, no reference data, no internal AI metadata")
    print(f"   Ready: For WAV synthesis program with user layering control")


if __name__ == "__main__":
    main()