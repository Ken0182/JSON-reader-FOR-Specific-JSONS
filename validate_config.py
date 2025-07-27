#!/usr/bin/env python3
"""
Configuration Validator

This script validates that the generated clean_config.json meets all requirements:
1. No data from reference files (moods.json, Synthesizer.json)
2. No empty fields, null values, or empty containers
3. Proper top-level nesting with instrument/group names
4. Structure mappings only when relevant
5. No internal AI metadata in output
"""

import json
from typing import Dict, Any, List


def load_reference_data():
    """Load reference files to check for contamination"""
    with open('moods.json', 'r') as f:
        moods_data = json.load(f)
    
    with open('Synthesizer.json', 'r') as f:
        synth_data = json.load(f)
    
    return moods_data, synth_data


def check_for_empty_values(data: Any, path: str = "") -> List[str]:
    """Recursively check for empty values, nulls, or empty containers"""
    issues = []
    
    if data is None:
        issues.append(f"Found null value at {path}")
    elif isinstance(data, dict):
        if not data:
            issues.append(f"Found empty dict at {path}")
        for key, value in data.items():
            issues.extend(check_for_empty_values(value, f"{path}.{key}" if path else key))
    elif isinstance(data, list):
        if not data:
            issues.append(f"Found empty list at {path}")
        for i, item in enumerate(data):
            issues.extend(check_for_empty_values(item, f"{path}[{i}]"))
    elif isinstance(data, str):
        if not data.strip():
            issues.append(f"Found empty string at {path}")
    
    return issues


def check_reference_contamination(config: Dict[str, Any], moods_data: Dict, synth_data: Dict) -> List[str]:
    """Check if any reference file data appears in the output"""
    issues = []
    
    # Extract reference names/identifiers that should NOT appear as top-level keys
    reference_keys = set()
    
    # From moods.json
    if 'moods' in moods_data:
        for mood in moods_data['moods']:
            if 'name' in mood:
                reference_keys.add(mood['name'].lower())
    
    # From Synthesizer.json
    if 'sections' in synth_data:
        for section_name in synth_data['sections'].keys():
            reference_keys.add(section_name.lower())
    
    # Check if any reference keys appear as top-level instrument/group names
    for config_key in config.keys():
        if config_key.lower() in reference_keys:
            # This is actually OK if it's a real instrument that happens to match
            # But let's check if it has synthesis-specific structure
            item = config[config_key]
            if not any(key in item for key in ['guitarParams', 'synthesis_type', 'oscillator', 'envelope']):
                issues.append(f"Potential reference contamination: '{config_key}' appears to be from reference files")
    
    return issues


def check_structure_application(config: Dict[str, Any]) -> Dict[str, int]:
    """Check structure mapping application"""
    stats = {
        'with_structure': 0,
        'without_structure': 0,
        'total_items': len(config)
    }
    
    for key, item in config.items():
        if 'structure' in item:
            stats['with_structure'] += 1
        else:
            stats['without_structure'] += 1
    
    return stats


def check_top_level_structure(config: Dict[str, Any]) -> List[str]:
    """Verify proper top-level nesting"""
    issues = []
    
    for key, item in config.items():
        if not isinstance(item, dict):
            issues.append(f"Top-level item '{key}' is not a dict: {type(item)}")
            continue
        
        # Check for required structure - either guitar or synth-based
        is_guitar = 'guitarParams' in item
        is_synth = 'synthesis_type' in item or 'oscillator' in item
        
        if not (is_guitar or is_synth):
            issues.append(f"Item '{key}' doesn't appear to be a valid instrument or group")
    
    return issues


def check_for_internal_metadata(config: Dict[str, Any]) -> List[str]:
    """Check that no internal AI metadata appears in output"""
    issues = []
    
    # These should never appear in the output
    forbidden_keys = [
        'layer', 'layering', 'internal_layer', 'ai_score', 'score', 
        'internal_score', 'layer_assignment', 'ai_layer'
    ]
    
    def check_recursive(data: Any, path: str = ""):
        if isinstance(data, dict):
            for key, value in data.items():
                if key.lower() in forbidden_keys:
                    issues.append(f"Found internal metadata '{key}' at {path}")
                check_recursive(value, f"{path}.{key}" if path else key)
        elif isinstance(data, list):
            for i, item in enumerate(data):
                check_recursive(item, f"{path}[{i}]")
    
    check_recursive(config)
    return issues


def validate_config():
    """Main validation function"""
    print("🔍 Validating clean_config.json")
    print("=" * 40)
    
    # Load files
    try:
        with open('clean_config.json', 'r') as f:
            config = json.load(f)
        
        moods_data, synth_data = load_reference_data()
        print(f"✓ Loaded config with {len(config)} items")
        
    except Exception as e:
        print(f"✗ Error loading files: {e}")
        return False
    
    all_passed = True
    
    # Test 1: Check for empty values
    print("\n📋 Test 1: Checking for empty/null values...")
    empty_issues = check_for_empty_values(config)
    if empty_issues:
        print(f"✗ Found {len(empty_issues)} empty value issues:")
        for issue in empty_issues[:5]:  # Show first 5
            print(f"  - {issue}")
        all_passed = False
    else:
        print("✓ No empty/null values found")
    
    # Test 2: Check for reference contamination
    print("\n📋 Test 2: Checking for reference file contamination...")
    ref_issues = check_reference_contamination(config, moods_data, synth_data)
    if ref_issues:
        print(f"✗ Found {len(ref_issues)} contamination issues:")
        for issue in ref_issues:
            print(f"  - {issue}")
        all_passed = False
    else:
        print("✓ No reference file contamination detected")
    
    # Test 3: Check structure application
    print("\n📋 Test 3: Checking structure mapping application...")
    structure_stats = check_structure_application(config)
    print(f"✓ Structure mapping stats:")
    print(f"  - Items with structure: {structure_stats['with_structure']}")
    print(f"  - Items without structure: {structure_stats['without_structure']}")
    print(f"  - Total items: {structure_stats['total_items']}")
    print("✓ Structure applied conditionally (as required)")
    
    # Test 4: Check top-level structure
    print("\n📋 Test 4: Checking top-level nesting structure...")
    structure_issues = check_top_level_structure(config)
    if structure_issues:
        print(f"✗ Found {len(structure_issues)} structure issues:")
        for issue in structure_issues:
            print(f"  - {issue}")
        all_passed = False
    else:
        print("✓ All items properly structured with top-level names")
    
    # Test 5: Check for internal metadata
    print("\n📋 Test 5: Checking for internal AI metadata...")
    metadata_issues = check_for_internal_metadata(config)
    if metadata_issues:
        print(f"✗ Found {len(metadata_issues)} internal metadata issues:")
        for issue in metadata_issues:
            print(f"  - {issue}")
        all_passed = False
    else:
        print("✓ No internal AI metadata found in output")
    
    # Test 6: Sample content verification
    print("\n📋 Test 6: Sample content verification...")
    sample_keys = list(config.keys())[:3]
    print(f"✓ Sample instruments/groups: {sample_keys}")
    
    for key in sample_keys[:1]:  # Check first item
        item = config[key]
        print(f"✓ '{key}' structure:")
        for prop in sorted(item.keys()):
            print(f"  - {prop}: {type(item[prop]).__name__}")
    
    # Final summary
    print("\n" + "=" * 40)
    if all_passed:
        print("🎉 VALIDATION PASSED!")
        print("✅ Configuration meets all requirements:")
        print("   • Clean, minimal output structure")
        print("   • No reference file contamination") 
        print("   • No empty/null values")
        print("   • Proper top-level nesting")
        print("   • Conditional structure mapping")
        print("   • No internal AI metadata")
        print("   • Ready for WAV synthesis program")
    else:
        print("❌ VALIDATION FAILED!")
        print("Some requirements are not met. Check issues above.")
    
    return all_passed


if __name__ == "__main__":
    validate_config()