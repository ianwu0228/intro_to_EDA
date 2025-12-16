# import sys
# import matplotlib.pyplot as plt
# import matplotlib.patches as patches
# import argparse
# import random

# def parse_input_file(filepath):
#     """Parses the input file to get Chip dimensions and Fixed Modules."""
#     chip_width = 0
#     chip_height = 0
#     fixed_modules = []
    
#     try:
#         with open(filepath, 'r') as f:
#             lines = f.read().split()
            
#         i = 0
#         while i < len(lines):
#             token = lines[i]
            
#             if token == "CHIP":
#                 chip_width = int(lines[i+1])
#                 chip_height = int(lines[i+2])
#                 i += 3
                
#             elif token == "FIXEDMODULE":
#                 num_fixed = int(lines[i+1])
#                 i += 2
#                 for _ in range(num_fixed):
#                     name = lines[i]
#                     x = int(lines[i+1])
#                     y = int(lines[i+2])
#                     w = int(lines[i+3])
#                     h = int(lines[i+4])
#                     fixed_modules.append({
#                         'name': name,
#                         'x': x, 'y': y,
#                         'w': w, 'h': h
#                     })
#                     i += 5
#             elif token == "SOFTMODULE":
#                 # Skip soft modules in input file (we get their positions from output)
#                 num_soft = int(lines[i+1])
#                 i += 2 + (num_soft * 2) # Skip name and area for each
#             elif token == "CONNECTION":
#                 break # We don't need connections for plotting
#             else:
#                 i += 1
                
#     except Exception as e:
#         print(f"Error parsing input file: {e}")
#         sys.exit(1)
        
#     return chip_width, chip_height, fixed_modules

# def parse_output_file(filepath):
#     """Parses the output file to get HPWL and Soft Module coordinates."""
#     soft_modules = []
#     hpwl = 0.0
    
#     try:
#         with open(filepath, 'r') as f:
#             lines = f.read().split()
            
#         i = 0
#         while i < len(lines):
#             token = lines[i]
            
#             if token == "HPWL":
#                 hpwl = float(lines[i+1])
#                 i += 2
                
#             elif token == "SOFTMODULE":
#                 num_soft = int(lines[i+1])
#                 i += 2
#                 for _ in range(num_soft):
#                     name = lines[i]
#                     num_corners = int(lines[i+1])
#                     i += 2
#                     corners = []
#                     for _ in range(num_corners):
#                         cx = int(lines[i])
#                         cy = int(lines[i+1])
#                         corners.append((cx, cy))
#                         i += 2
#                     soft_modules.append({
#                         'name': name,
#                         'corners': corners
#                     })
#             else:
#                 i += 1
                
#     except Exception as e:
#         print(f"Error parsing output file: {e}")
#         sys.exit(1)
        
#     return hpwl, soft_modules

# def plot_result(chip_w, chip_h, fixed_modules, soft_modules, hpwl, output_img="result.png"):
#     fig, ax = plt.subplots(figsize=(12, 10))
    
#     # 1. Draw Chip Boundary
#     chip_rect = patches.Rectangle((0, 0), chip_w, chip_h, 
#                                   linewidth=2, edgecolor='black', facecolor='none', linestyle='--')
#     ax.add_patch(chip_rect)
    
#     # 2. Draw Fixed Modules (Uniform Color: Dark Grey)
#     for mod in fixed_modules:
#         rect = patches.Rectangle((mod['x'], mod['y']), mod['w'], mod['h'],
#                                  linewidth=1, edgecolor='black', facecolor='#404040', alpha=0.9)
#         ax.add_patch(rect)
#         # Label
#         cx = mod['x'] + mod['w']/2
#         cy = mod['y'] + mod['h']/2
#         ax.text(cx, cy, mod['name'], color='white', ha='center', va='center', fontsize=8, fontweight='bold')

#     # 3. Draw Soft Modules (Distinct Colors)
#     cmap = plt.cm.get_cmap('tab20')
#     for idx, mod in enumerate(soft_modules):
#         corners = mod['corners']
#         poly = patches.Polygon(corners, closed=True,
#                                linewidth=1, edgecolor='black', 
#                                facecolor=cmap(idx % 20), alpha=0.7)
#         ax.add_patch(poly)
        
#         # Calculate centroid for label
#         xs = [c[0] for c in corners]
#         ys = [c[1] for c in corners]
#         cx = sum(xs) / len(xs)
#         cy = sum(ys) / len(ys)
#         ax.text(cx, cy, mod['name'], color='black', ha='center', va='center', fontsize=8)

#     # Settings
#     ax.set_xlim(-chip_w*0.05, chip_w*1.05)
#     ax.set_ylim(-chip_h*0.05, chip_h*1.05)
#     ax.set_aspect('equal')
#     ax.set_title(f"Floorplan Result (HPWL: {hpwl:.1f})\nFixed: Grey | Soft: Colored", fontsize=14)
#     ax.grid(True, linestyle=':', alpha=0.3)
    
#     plt.savefig(output_img, dpi=300)
#     print(f"Plot saved to {output_img}")
#     # plt.show() # Commented out to run cleanly on servers without display

# if __name__ == "__main__":
#     parser = argparse.ArgumentParser(description='Visualize ICCAD Floorplan Result')
#     parser.add_argument('input_file', help='Path to the input problem file (.txt)')
#     parser.add_argument('output_file', help='Path to the solver output file (.txt)')
#     parser.add_argument('-o', '--plot_file', default='result.png', help='Output filename for the plot image (default: result.png)')
    
#     args = parser.parse_args()
    
#     # Parse
#     cw, ch, fixed = parse_input_file(args.input_file)
#     hpwl, soft = parse_output_file(args.output_file)
    
#     print(f"Chip Size: {cw} x {ch}")
#     print(f"Fixed Modules: {len(fixed)}")
#     print(f"Soft Modules: {len(soft)}")
    
#     # Plot
#     plot_result(cw, ch, fixed, soft, hpwl, args.plot_file)

import sys
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import argparse
import random

def parse_input_file(filepath):
    """Parses the input file to get Chip dimensions, Fixed Modules, and Connections."""
    chip_width = 0
    chip_height = 0
    fixed_modules = []
    connections = []
    
    try:
        with open(filepath, 'r') as f:
            lines = f.read().split()
            
        i = 0
        while i < len(lines):
            token = lines[i]
            
            if token == "CHIP":
                chip_width = int(lines[i+1])
                chip_height = int(lines[i+2])
                i += 3
                
            elif token == "FIXEDMODULE":
                num_fixed = int(lines[i+1])
                i += 2
                for _ in range(num_fixed):
                    name = lines[i]
                    x = int(lines[i+1])
                    y = int(lines[i+2])
                    w = int(lines[i+3])
                    h = int(lines[i+4])
                    fixed_modules.append({
                        'name': name,
                        'x': x, 'y': y,
                        'w': w, 'h': h
                    })
                    i += 5
            elif token == "SOFTMODULE":
                # Skip soft modules in input file (we get their positions from output)
                num_soft = int(lines[i+1])
                i += 2 + (num_soft * 2) # Skip name and area for each
            elif token == "CONNECTION":
                num_connections = int(lines[i+1])
                i += 2
                for _ in range(num_connections):
                    mod1 = lines[i]
                    mod2 = lines[i+1]
                    weight = int(lines[i+2])
                    connections.append({
                        'source': mod1,
                        'target': mod2,
                        'weight': weight
                    })
                    i += 3
            else:
                i += 1
                
    except Exception as e:
        print(f"Error parsing input file: {e}")
        sys.exit(1)
        
    return chip_width, chip_height, fixed_modules, connections

def parse_output_file(filepath):
    """Parses the output file to get HPWL and Soft Module coordinates."""
    soft_modules = []
    hpwl = 0.0
    
    try:
        with open(filepath, 'r') as f:
            lines = f.read().split()
            
        i = 0
        while i < len(lines):
            token = lines[i]
            
            if token == "HPWL":
                hpwl = float(lines[i+1])
                i += 2
                
            elif token == "SOFTMODULE":
                num_soft = int(lines[i+1])
                i += 2
                for _ in range(num_soft):
                    name = lines[i]
                    num_corners = int(lines[i+1])
                    i += 2
                    corners = []
                    for _ in range(num_corners):
                        cx = int(lines[i])
                        cy = int(lines[i+1])
                        corners.append((cx, cy))
                        i += 2
                    soft_modules.append({
                        'name': name,
                        'corners': corners
                    })
            else:
                i += 1
                
    except Exception as e:
        print(f"Error parsing output file: {e}")
        sys.exit(1)
        
    return hpwl, soft_modules

def get_module_centers(fixed_modules, soft_modules):
    """Helper to create a dictionary mapping module names to their center (x, y)."""
    centers = {}
    
    # Calculate Fixed Module Centers
    for mod in fixed_modules:
        cx = mod['x'] + mod['w'] / 2.0
        cy = mod['y'] + mod['h'] / 2.0
        centers[mod['name']] = (cx, cy)
        
    # Calculate Soft Module Centers (Centroid of polygon)
    for mod in soft_modules:
        corners = mod['corners']
        xs = [c[0] for c in corners]
        ys = [c[1] for c in corners]
        cx = sum(xs) / len(xs)
        cy = sum(ys) / len(ys)
        centers[mod['name']] = (cx, cy)
        
    return centers

def plot_result(chip_w, chip_h, fixed_modules, soft_modules, connections, hpwl, output_img="result.png"):
    fig, ax = plt.subplots(figsize=(12, 10))
    
    # 1. Draw Chip Boundary
    chip_rect = patches.Rectangle((0, 0), chip_w, chip_h, 
                                  linewidth=2, edgecolor='black', facecolor='none', linestyle='--', zorder=5)
    ax.add_patch(chip_rect)
    
    # 2. Draw Fixed Modules
    for mod in fixed_modules:
        rect = patches.Rectangle((mod['x'], mod['y']), mod['w'], mod['h'],
                                 linewidth=1, edgecolor='black', facecolor='#404040', alpha=0.9, zorder=3)
        ax.add_patch(rect)
        # Label
        cx = mod['x'] + mod['w']/2
        cy = mod['y'] + mod['h']/2
        ax.text(cx, cy, mod['name'], color='white', ha='center', va='center', fontsize=8, fontweight='bold', zorder=6)

    # 3. Draw Soft Modules
    cmap = plt.cm.get_cmap('tab20')
    for idx, mod in enumerate(soft_modules):
        corners = mod['corners']
        poly = patches.Polygon(corners, closed=True,
                               linewidth=1, edgecolor='black', 
                               facecolor=cmap(idx % 20), alpha=0.7, zorder=3)
        ax.add_patch(poly)
        
        # Calculate centroid for label
        xs = [c[0] for c in corners]
        ys = [c[1] for c in corners]
        cx = sum(xs) / len(xs)
        cy = sum(ys) / len(ys)
        ax.text(cx, cy, mod['name'], color='black', ha='center', va='center', fontsize=8, zorder=6)

    # 4. Draw Connections (NEW)
    module_centers = get_module_centers(fixed_modules, soft_modules)
    
    if connections:
        # Determine max weight for scaling line widths
        max_weight = max(c['weight'] for c in connections)
        
        for conn in connections:
            u, v = conn['source'], conn['target']
            w = conn['weight']
            
            if u in module_centers and v in module_centers:
                ux, uy = module_centers[u]
                vx, vy = module_centers[v]
                
                # Scale line width: Base 0.5 + proportional thickness up to 3.5 total
                scaled_width = 0.5 + (w / max_weight) * 3.0
                
                ax.plot([ux, vx], [uy, vy], color='black', linewidth=scaled_width, alpha=0.6, zorder=4)

    # Settings
    ax.set_xlim(-chip_w*0.05, chip_w*1.05)
    ax.set_ylim(-chip_h*0.05, chip_h*1.05)
    ax.set_aspect('equal')
    ax.set_title(f"Floorplan Result (HPWL: {hpwl:.1f})\nFixed: Grey | Soft: Colored | Lines: Connections", fontsize=14)
    ax.grid(True, linestyle=':', alpha=0.3, zorder=0)
    
    plt.savefig(output_img, dpi=300)
    print(f"Plot saved to {output_img}")
    # plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Visualize ICCAD Floorplan Result')
    parser.add_argument('input_file', help='Path to the input problem file (.txt)')
    parser.add_argument('output_file', help='Path to the solver output file (.txt)')
    parser.add_argument('-o', '--plot_file', default='result.png', help='Output filename for the plot image (default: result.png)')
    
    args = parser.parse_args()
    
    # Parse
    cw, ch, fixed, connections = parse_input_file(args.input_file)
    hpwl, soft = parse_output_file(args.output_file)
    
    print(f"Chip Size: {cw} x {ch}")
    print(f"Fixed Modules: {len(fixed)}")
    print(f"Soft Modules: {len(soft)}")
    print(f"Connections: {len(connections)}")
    
    # Plot
    plot_result(cw, ch, fixed, soft, connections, hpwl, args.plot_file)