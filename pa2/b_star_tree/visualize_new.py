import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

def visualize_floorplan(filename):
    # Read the file
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    # Parse outline dimensions
    outline_w, outline_h = map(int, lines[0].strip().split()[1:])
    
    # Skip metrics lines (cost, wirelength, area, dimensions, runtime)
    blocks_start = 6
    blocks = []
    
    # Parse block information
    for line in lines[blocks_start:]:
        name, x1, y1, x2, y2 = line.strip().split()
        blocks.append({
            'name': name,
            'x1': int(x1),
            'y1': int(y1),
            'x2': int(x2),
            'y2': int(y2)
        })
    
    # Create figure and axis
    fig, ax = plt.subplots(figsize=(8, 6))
    
    # Draw outline
    outline = patches.Rectangle((0, 0), outline_w, outline_h, 
                              fill=False, color='red', 
                              linewidth=2, linestyle='--',
                              label='Outline')
    ax.add_patch(outline)
    
    # Draw blocks
    colors = plt.cm.Set3(np.linspace(0, 1, len(blocks)))
    for block, color in zip(blocks, colors):
        # Create rectangle
        width = block['x2'] - block['x1']
        height = block['y2'] - block['y1']
        rect = patches.Rectangle((block['x1'], block['y1']), width, height,
                               facecolor=color, edgecolor='black',
                               alpha=0.7)
        ax.add_patch(rect)
        
        # Add block name at center
        cx = block['x1'] + width/2
        cy = block['y1'] + height/2
        ax.text(cx, cy, block['name'], 
                horizontalalignment='center',
                verticalalignment='center')
    
    # Set axis limits with some padding
    max_x = max(outline_w, max(b['x2'] for b in blocks))
    max_y = max(outline_h, max(b['y2'] for b in blocks))
    padding = max(max_x, max_y) * 0.05
    
    ax.set_xlim(-padding, max_x + padding)
    ax.set_ylim(-padding, max_y + padding)
    
    # Set labels and title
    ax.set_xlabel('Width')
    ax.set_ylabel('Height')
    ax.set_title('Floorplan Visualization')
    
    # Add legend
    ax.legend()
    
    # Set aspect ratio to equal
    ax.set_aspect('equal')
    
    # Show grid
    ax.grid(True, linestyle=':')
    plt.savefig('floorplan_visualization.png', dpi=900, bbox_inches='tight')
    # Show plot
    plt.show()

# Usage
visualize_floorplan('test.txt')