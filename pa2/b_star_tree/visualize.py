# import matplotlib.pyplot as plt

# def draw_blocks(file_path):
#     with open(file_path, 'r') as f:
#         for line in f:
#             tokens = line.strip().split()
#             name, x1, y1, x2, y2, rot = tokens
#             x1, y1, x2, y2 = map(int, [x1, y1, x2, y2])
#             width = x2 - x1
#             height = y2 - y1
#             plt.gca().add_patch(
#                 plt.Rectangle((x1, y1), width, height,
#                               edgecolor='black', facecolor='yellow', fill=True)
#             )
#             plt.text(x1 + width / 2, y1 + height / 2, name,
#                      ha='center', va='center', fontsize=8)

#     plt.axis('equal')
#     plt.grid(True)
#     plt.title("B*-Tree Floorplan Visualization")
#     plt.savefig("floorplan.png")
#     plt.show()

# # Usage
# # draw_blocks("./initial.txt")
# draw_blocks("./output.txt")

import matplotlib.pyplot as plt

def visualize(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()

    # Parse outline from first line
    if not lines[0].startswith("OUTLINE"):
        raise ValueError("First line must specify outline: OUTLINE width height")

    _, w, h = lines[0].split()
    outline_w = int(w)
    outline_h = int(h)

    fig, ax = plt.subplots()

    # Draw fixed outline as dashed rectangle
    outline_rect = plt.Rectangle((0, 0), outline_w, outline_h,
                                 linewidth=2, edgecolor='red', facecolor='none', linestyle='--')
    ax.add_patch(outline_rect)

    # Draw blocks
    for line in lines[1:]:
        parts = line.strip().split()
        if len(parts) < 6: continue
        name, x1, y1, x2, y2, rotation = parts
        x1, y1, x2, y2 = map(int, [x1, y1, x2, y2])

        width = x2 - x1
        height = y2 - y1

        rect = plt.Rectangle((x1, y1), width, height,
                             linewidth=1, edgecolor='black', facecolor='yellow', alpha=0.5)
        ax.add_patch(rect)
        ax.text(x1 + width / 2, y1 + height / 2, name, fontsize=8, ha='center', va='center')

    ax.set_xlim(-100, outline_w + 100)
    ax.set_ylim(-100, outline_h + 100)
    ax.set_title("B*-Tree Floorplan Visualization")
    ax.set_aspect('equal')
    plt.savefig("floorplan.png")

    plt.show()

# Example usage:
visualize("output.txt")
