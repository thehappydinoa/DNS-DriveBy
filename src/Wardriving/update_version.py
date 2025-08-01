#!/usr/bin/env python3
"""
Update firmware version with git hash
"""

import subprocess
import re
import sys

def get_git_hash():
    """Get the current git hash (short version)"""
    try:
        result = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], 
                              capture_output=True, text=True, cwd='../..')
        if result.returncode == 0:
            return result.stdout.strip()
        else:
            return "unknown"
    except:
        return "unknown"

def update_version_file():
    """Update the version in Wardriving.ino"""
    git_hash = get_git_hash()
    
    # Read the current file
    with open('Wardriving.ino', 'r') as f:
        content = f.read()
    
    # Update the version line
    # Look for the FIRMWARE_VERSION line and update it
    pattern = r'#define FIRMWARE_VERSION "([^"]+)"'
    replacement = f'#define FIRMWARE_VERSION "2.2.0-{git_hash}"'
    
    new_content = re.sub(pattern, replacement, content)
    
    # Write back to file
    with open('Wardriving.ino', 'w') as f:
        f.write(new_content)
    
    print(f"Updated firmware version to 2.2.0-{git_hash}")
    return f"2.2.0-{git_hash}"

if __name__ == "__main__":
    version = update_version_file()
    print(f"Version updated: {version}") 