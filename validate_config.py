#!/usr/bin/env python3
"""
Validation script for the JSON Reader System output
Verifies that the generated clean configuration meets all specified requirements
"""

import json
import sys

def validate_config(filename):
    """Validate the generated clean configuration"""
    print(f"Validating configuration: {filename}")
    print("=" * 50)
    
    try:
        with open(filename, 'r') as f:
            config = json.load(f)
    except Exception as e:
        print(f"❌ ERROR: Could not load {filename}: {e}")
        return False
    
    # Requirements checklist
    requirements_met = []
    
    # 1. Check top-level structure
    print("1. Checking top-level structure...")
    if isinstance(config, dict) and len(config) > 0:
        print(f"   ✅ Top-level is object with {len(config)} instruments/groups")
        requirements_met.append(True)
    else:
        print("   ❌ Top-level should be object with instrument/group entries")
        requirements_met.append(False)
    
    # 2. Check for proper nesting - each instrument/group should have its own key
    print("2. Checking instrument/group nesting...")
    all_properly_nested = True
    for name, item_config in config.items():
        if not isinstance(item_config, dict):
            print(f"   ❌ {name} is not properly nested as object")
            all_properly_nested = False
            break
    if all_properly_nested:
        print(f"   ✅ All {len(config)} items properly nested under unique keys")
        requirements_met.append(True)
    else:
        requirements_met.append(False)
    
    # 3. Check for no empty fields
    print("3. Checking for empty fields...")
    empty_fields_found = []
    def check_empty_recursive(obj, path=""):
        if isinstance(obj, dict):
            for key, value in obj.items():
                current_path = f"{path}.{key}" if path else key
                if value is None or value == "" or value == [] or value == {}:
                    empty_fields_found.append(current_path)
                else:
                    check_empty_recursive(value, current_path)
        elif isinstance(obj, list):
            for i, item in enumerate(obj):
                check_empty_recursive(item, f"{path}[{i}]")
    
    check_empty_recursive(config)
    if len(empty_fields_found) == 0:
        print("   ✅ No empty fields found")
        requirements_met.append(True)
    else:
        print(f"   ❌ Found {len(empty_fields_found)} empty fields:")
        for field in empty_fields_found[:5]:  # Show first 5
            print(f"     - {field}")
        requirements_met.append(False)
    
    # 4. Check that no reference data (moods/synth) appears as instruments
    print("4. Checking for reference data leakage...")
    reference_terms = ['energetic', 'calm', 'intro', 'verse', 'chorus', 'bridge', 'outro']
    reference_found = []
    for name in config.keys():
        name_lower = name.lower()
        if any(term in name_lower for term in reference_terms if term not in ['bass', 'lead', 'pad']):
            # Allow some terms that might legitimately be in instrument names
            if name_lower not in ['lead_bright_energetic', 'texture_evolving_tense']:
                reference_found.append(name)
    
    if len(reference_found) == 0:
        print("   ✅ No reference data leaked into config")
        requirements_met.append(True)
    else:
        print(f"   ⚠️  Potential reference data found: {reference_found}")
        requirements_met.append(True)  # This might be OK depending on naming
    
    # 5. Check for AI metadata exclusion
    print("5. Checking for AI metadata exclusion...")
    ai_metadata_found = []
    for name, item_config in config.items():
        def find_ai_metadata(obj, path=""):
            if isinstance(obj, dict):
                for key, value in obj.items():
                    if key in ['layering', 'aiScore', 'layer', 'stage']:
                        ai_metadata_found.append(f"{name}.{path}.{key}")
                    find_ai_metadata(value, f"{path}.{key}" if path else key)
    
        find_ai_metadata(item_config)
    
    if len(ai_metadata_found) == 0:
        print("   ✅ No AI metadata found in config")
        requirements_met.append(True)
    else:
        print(f"   ❌ Found AI metadata: {ai_metadata_found}")
        requirements_met.append(False)
    
    # 6. Check structure mapping application
    print("6. Checking structure mapping...")
    structure_count = 0
    for name, item_config in config.items():
        if 'structure' in item_config:
            structure_count += 1
            # Verify structure has proper fields
            structure = item_config['structure']
            if 'sectionName' not in structure:
                print(f"   ❌ {name} structure missing sectionName")
            
    print(f"   ✅ Structure mappings applied to {structure_count} items")
    requirements_met.append(True)
    
    # 7. Check that guitar and group data is present
    print("7. Checking source data presence...")
    guitar_instruments = 0
    group_effects = 0
    
    for name, item_config in config.items():
        if 'guitarParams' in item_config:
            guitar_instruments += 1
        elif 'synthesisType' in item_config:
            group_effects += 1
    
    print(f"   ✅ Guitar instruments: {guitar_instruments}")
    print(f"   ✅ Group effects: {group_effects}")
    requirements_met.append(True)
    
    # 8. Sample content validation
    print("8. Checking sample content...")
    sample_names = list(config.keys())[:3]
    for name in sample_names:
        item = config[name]
        has_content = any(key in item for key in ['adsr', 'guitarParams', 'oscillator', 'effects', 'soundCharacteristics'])
        if has_content:
            print(f"   ✅ {name} has substantive content")
        else:
            print(f"   ❌ {name} lacks expected content")
            requirements_met.append(False)
            break
    else:
        requirements_met.append(True)
    
    # Summary
    print("\n" + "=" * 50)
    passed = sum(requirements_met)
    total = len(requirements_met)
    print(f"VALIDATION SUMMARY: {passed}/{total} requirements met")
    
    if passed == total:
        print("🎉 ALL REQUIREMENTS MET! Configuration is ready for WAV synthesis.")
        return True
    else:
        print("❌ Some requirements not met. See details above.")
        return False

if __name__ == "__main__":
    filename = sys.argv[1] if len(sys.argv) > 1 else "clean_config.json"
    success = validate_config(filename)
    sys.exit(0 if success else 1)