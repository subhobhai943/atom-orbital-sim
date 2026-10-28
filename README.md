# Quantum Hydrogen Atom & Orbital Simulator (C++ & OpenGL)

A physically accurate, interactive 3D quantum mechanical atom and electron orbital simulation written in modern C++ (C++17) and OpenGL 3.3 Core, inspired by Kavan G's video *"Simulating Atoms in C++"*.

Moving far beyond naive 2D Lewis structures or outdated Bohr planetary orbits, this simulation solves the **Time-Independent Schrödinger Equation** for hydrogen-like atoms and visualizes electron probability density distributions $|\psi_{nlm}(r, \theta, \phi)|^2$.

---

## Features

### 1. 3D Realtime Electron Cloud (Monte Carlo & CDF Inversion)
- Generates up to **500,000 electron probability points** sampled directly from $|\psi_{nlm}|^2$ using analytical CDF-inversion samplers.
- **Quantum Probability Current ($\mathbf{j}$)**: For complex eigenstates with $m \neq 0$, simulates the azimuthal probability current velocity $v_\phi = \frac{\hbar m}{m_e r \sin\theta}$, creating a dynamic circulating quantum vortex around the polar axis.
- **Soft Glowing Particles**: Custom GLSL shader with distance attenuation and Gaussian falloff $\exp(-4r^2)$ for ethereal luminous nebular clouds without square artifacts.
- **Nucleus Rendering**: Multi-particle proton/neutron central cluster with Blinn-Phong lighting and a pulsing quantum core glow.
- **Internal Cross-Section Slicing**: Dynamic clipping planes along X, Y, or Z axes to look inside orbital shells and observe radial and angular nodes.

### 2. 3D Volumetric Wavefunction Raymarching (GPU Shader)
- Screen-filling volumetric raymarcher that evaluates the exact analytical hydrogen wavefunctions directly on the GPU.
- Beer-Lambert absorption and volumetric emission integration along each camera ray.
- Continuous, silky-smooth volumetric gas cloud rendering with zero point noise.

### 3. Quantum Excitation & Transition Jumps (Photon Emission & Absorption)
- Real-time simulation of atomic transitions between energy levels $E_n = -\frac{13.606\text{ eV}}{n^2}$.
- Computes exact transition energies $\Delta E$ and photon wavelengths $\lambda = \frac{hc}{\Delta E}$ in nanometers.
- **Accurate Spectral Colors**: Generates physical RGB colors (e.g., Balmer $H_\alpha$ at $656.3\text{ nm}$ in radiant red, $H_\beta$ at $486.1\text{ nm}$ in luminous cyan, Lyman $\alpha$ at $121.6\text{ nm}$ in ultraviolet).
- **Time-Dependent Superposition**: Animates the quantum superposition state $\Psi(\mathbf{r}, t) = c_1(t)\psi_1 + c_2(t)\psi_2 e^{-i \Delta E t / \hbar}$, revealing the oscillating electron dipole beating at the Bohr frequency during quantum jumps.
- **Traveling Electromagnetic Wavepackets**: Renders oscillating sinusoidal wavepackets with Gaussian envelopes arriving or departing during absorption/emission.

### 4. 2D Wavefunction Slice & Nodal Surface Visualizer
- High-resolution 2D cross-section plane showing the sign and amplitude of $\psi(\mathbf{r})$.
- Clear visual distinction between positive phase lobes (warm amber) and negative phase lobes (electric cyan).
- **Nodal Lines Highlight**: Accurately highlights surfaces where $\psi = 0$ ($n - l - 1$ radial nodal rings and $l$ angular nodal lines).
- Iso-probability contour lines for probability gradients.

### 5. 3D Bohr Planetary Model vs. Quantum Mechanics Comparison
- Side-by-side or overlay comparison between classical Bohr circular orbits ($r_n = n^2 a_0$) and modern quantum clouds.
- **de Broglie Standing Matter Wave**: Renders the standing electron wave undulating around the circular orbit satisfying $2\pi r = n \lambda_{\text{dB}}$.
- Explains why classical planetary orbits fail (Larmor radiation energy loss, Heisenberg uncertainty principle).

### 6. Interactive Dear ImGui Dashboard
- Real-time sliders for quantum numbers:
  - Principal $n \in [1, 6]$
  - Azimuthal $l \in [0, n - 1]$ ($s, p, d, f, g$)
  - Magnetic $m \in [-l, +l]$
- **Real Chemistry Orbitals** ($p_x, p_y, p_z$, $d_{z^2}, d_{xz}, d_{yz}, d_{xy}, d_{x^2-y^2}$, $f$) vs. **Complex Eigenstates** ($e^{im\phi}$).
- **One-Click Presets**: `1s Ground`, `2s Shell`, `2pz Dumbbell`, `2px Lobe`, `3dz² Donut`, `3dxy Clover`, `4fz³ Flower`.
- Multiple Color Palettes:
  1. *Thermal Fire / Blackbody* (Kavan G classic)
  2. *Neon Cyan / Quantum Electric*
  3. *Quantum Phase (+Red / -Blue)*
  4. *Plasma / Synthwave*
  5. *Deep Cosmic Nebula*
- Quantum Physics Telemetry: exact energy level $E_n$, classical Bohr radius $r_n$, quantum mean radius $\langle r \rangle$, angular momentum magnitude $|L| = \sqrt{l(l+1)}\hbar$, projected component $L_z = m\hbar$, and nodal surface counts.

---

## Mathematical Foundations

### The Schrödinger Equation for Hydrogen
$$-\frac{\hbar^2}{2\mu} \nabla^2 \psi(\mathbf{r}) - \frac{e^2}{4\pi \varepsilon_0 r} \psi(\mathbf{r}) = E \psi(\mathbf{r})$$

In spherical coordinates $(r, \theta, \phi)$, the separable solution is:
$$\psi_{nlm}(r, \theta, \phi) = R_{nl}(r) Y_l^m(\theta, \phi)$$

#### 1. Radial Wavefunction $R_{nl}(r)$
$$R_{nl}(r) = \sqrt{\left(\frac{2}{n a_0}\right)^3 \frac{(n - l - 1)!}{2n (n + l)!}} \exp\left(-\frac{\rho}{2}\right) \rho^l L_{n - l - 1}^{2l + 1}(\rho)$$
where $\rho = \frac{2r}{n a_0}$, $a_0 \approx 1$ (Bohr radius), and $L_k^\alpha(x)$ are Generalized Laguerre Polynomials evaluated via the stable recurrence:
$$(j) L_j^\alpha(x) = (2j - 1 + \alpha - x) L_{j-1}^\alpha(x) - (j - 1 + \alpha) L_{j-2}^\alpha(x)$$

#### 2. Angular Wavefunction $Y_l^m(\theta, \phi)$
$$Y_l^m(\theta, \phi) = (-1)^m \sqrt{\frac{2l + 1}{4\pi} \frac{(l - m)!}{(l + m)!}} P_l^m(\cos\theta) e^{im\phi}$$
where $P_l^m(x)$ are Associated Legendre Polynomials evaluated using the stable Bonnet recurrence relation with Condon-Shortley phase.

#### 3. Real Chemistry Orbitals
For directional lobes:
$$Y_{lm, \text{cos}} \propto P_l^m(\cos\theta) \cos(m\phi), \quad Y_{lm, \text{sin}} \propto P_l^m(\cos\theta) \sin(|m|\phi)$$

---

## Directory Structure

```
atom-orbital-sim/
├── bin/                       # Compiled executables & shaders
│   ├── atom_orbital_sim       # Unified master application with Dear ImGui
│   ├── atom_realtime          # Standalone 3D point cloud visualizer
│   ├── atom_raytracer         # Standalone GPU volumetric raymarcher
│   ├── atom_excitation        # Standalone quantum transition simulator
│   ├── wave_atom_2d           # Standalone 2D wavefunction cross-section
│   ├── atom_bohr              # Standalone 3D Bohr model visualizer
│   ├── launcher               # Terminal menu launcher
│   └── test_orbital_math      # Analytical unit test suite
├── include/
│   ├── atom_state.h           # Simulation settings, quantum numbers, constants
│   ├── camera.h               # Smooth arcball camera (orbit, pan, zoom)
│   ├── color_maps.h           # Color ramps (Fire, Neon Cyan, Phase, Plasma, Nebula)
│   ├── gui.h                  # Dear ImGui controls & telemetry
│   ├── orbital_math.h         # Analytical Laguerre/Legendre/CDF samplers
│   ├── renderer_bohr.h        # 3D Bohr model renderer
│   ├── renderer_excitation.h  # Quantum jump & photon renderer
│   ├── renderer_particles.h   # 3D glowing point cloud renderer
│   ├── renderer_raytracer.h   # Volumetric raymarcher
│   ├── renderer_wave_2d.h     # 2D cross-section renderer
│   └── shader.h               # Shader compilation & uniform utilities
├── shaders/                   # GLSL 330 core shaders
│   ├── bohr.frag / bohr.vert
│   ├── nucleus.frag / nucleus.vert
│   ├── particle.frag / particle.vert
│   ├── photon.frag / photon.vert
│   ├── raymarch.frag / raymarch.vert
│   └── wave_slice.frag / wave_slice.vert
├── src/
│   ├── gui.cpp
│   ├── main.cpp
│   ├── renderer_bohr.cpp
│   ├── renderer_excitation.cpp
│   ├── renderer_particles.cpp
│   ├── renderer_raytracer.cpp
│   ├── renderer_wave_2d.cpp
│   └── standalone/            # Individual standalone main entry points
├── tests/
│   └── test_orbital_math.cpp  # Mathematical validation test suite
├── third_party/
│   └── imgui/                 # Dear ImGui library
└── CMakeLists.txt
```

---

## Building & Running

### Prerequisites (Ubuntu / Debian)
```bash
sudo apt update
sudo apt install -y build-essential cmake libglfw3-dev libglm-dev libglew-dev libgl1-mesa-dev
```

### Build Instructions
```bash
cd atom-orbital-sim
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)
```

All compiled binaries will be output directly into `atom-orbital-sim/bin/`.

---

## Running the Simulations

### 1. Unified Master Application (Recommended)
Run the master application with Dear ImGui to access all modes, presets, sliders, and telemetry simultaneously:
```bash
./bin/atom_orbital_sim
```

### 2. Standalone Executables
You can also run any standalone focused mode directly:
```bash
./bin/atom_realtime      # 3D Realtime Point Cloud
./bin/atom_raytracer     # GPU Volumetric Raymarcher
./bin/atom_excitation    # Quantum Jumps & Photon Emission
./bin/wave_atom_2d       # 2D Wavefunction Slice
./bin/atom_bohr          # 3D Bohr Model vs Quantum
```

### 3. Interactive Terminal Launcher
```bash
./bin/launcher
```

### 4. Run Unit Tests
```bash
./bin/test_orbital_math
```

---

## Keyboard & Mouse Controls

| Action | Control |
|---|---|
| **Orbit Camera** | Left Mouse Button + Drag |
| **Pan Camera** | Right Mouse Button + Drag |
| **Zoom View** | Mouse Scroll Wheel |
| **Reset Camera View** | `R` |
| **Toggle Auto-Rotation** | `Space` |
| **Switch Modes (Unified App)** | Keys `1` through `5` |
| **Adjust Quantum Numbers (Standalone)** | `Up`/`Down` ($n$), `Left`/`Right` ($l$), `M` ($m$) |
| **Cycle Color Palette** | `C` |
| **Exit** | `ESC` |

---

## References
- Kavan G's video: *"Simulating Atoms in C++"* (YouTube: `https://www.youtube.com/watch?v=OSAOh4L41Wg`)
- Original repository: [kavan010/Atoms](https://github.com/kavan010/Atoms)
- Griffiths, David J. *Introduction to Quantum Mechanics* (Pearson Prentice Hall).
- Cohen-Tannoudji, Claude et al. *Quantum Mechanics, Volume 1 & 2* (Wiley).
