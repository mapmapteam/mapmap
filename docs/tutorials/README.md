# MapMap Tutorials

Welcome to the MapMap tutorial collection! These tutorials will help you learn projection mapping with MapMap.

## Available Tutorials

### [Hello World Projection](HELLO_WORLD_PROJECTION.md)

**Difficulty**: Beginner
**Time**: 30-45 minutes
**Topics**: Paint, Mesh, Mapping, GPU Rendering

Learn the fundamentals of projection mapping by creating your first "Hello World" projection. This comprehensive tutorial covers:

- Understanding the Paint → Mapping → Mesh → Output pipeline
- Creating media sources (Paint)
- Defining warping geometry (Mesh)
- Connecting content to surfaces (Mapping)
- GPU-accelerated rendering with wgpu
- Running and testing your first projection

Perfect for developers new to projection mapping or the MapMap system.

## Prerequisites

Before starting these tutorials, ensure you have:

1. **Rust 1.75+** installed ([rustup.rs](https://rustup.rs))
2. **System dependencies** installed (see [BUILD.md](../../BUILD.md))
3. **MapMap repository** cloned and building successfully

## Tutorial Structure

Each tutorial follows this structure:

1. **Introduction** - What you'll learn and why it's important
2. **Prerequisites** - Required knowledge and tools
3. **Step-by-Step Guide** - Detailed implementation instructions
4. **Code Explanation** - Understanding what each part does
5. **Running the Example** - Testing your implementation
6. **Next Steps** - Where to go from here

## Getting Help

If you encounter issues with the tutorials:

1. Check the [BUILD.md](../../BUILD.md) for dependency installation
2. Review the [ARCHITECTURE.md](../ARCHITECTURE.md) for system design
3. Look at working examples in the `crates/mapmap/examples/` directory
4. Open an issue on [GitHub](https://github.com/johnjanik/mapmap/issues)

## Contributing Tutorials

Have an idea for a tutorial? Contributions are welcome! Please:

1. Follow the existing tutorial structure
2. Include working example code
3. Test on multiple platforms if possible
4. Submit a pull request

---

## Roadmap

Future tutorials planned:

- **Loading Videos and Images** - Working with real media files using FFmpeg
- **Advanced Mesh Warping** - Perspective correction and keystone adjustment
- **Multi-Projector Setup** - Edge blending and color calibration
- **Shader Effects** - Creating custom visual effects
- **Audio-Reactive Projections** - Syncing visuals to music
- **MIDI Control** - Live performance control
- **Creating a Complete Show** - From concept to execution

Stay tuned for more!

---

**Happy Learning! 🎨🔦**
