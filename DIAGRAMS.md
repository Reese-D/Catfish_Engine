# Catfish Engine — Architecture Diagrams

## 1. System Architecture

High-level view of the two binaries, the runner/interface layer, and shared game base.

```mermaid
graph TD
    subgraph client["catfish_client binary"]
        CE[main.cpp]
    end
    subgraph server["catfish_server binary"]
        SE[main.cpp]
    end

    subgraph runners["VulkanHelpers — Runners"]
        Engine["Engine\n(Vulkan + window loop)"]
        HeadlessRunner["HeadlessRunner\n(fixed 60 Hz tick, no GPU)"]
    end

    subgraph interfaces["VulkanHelpers — Interfaces"]
        IGame["IGame\ninitLogic / initGraphics\nupdate → FrameOutput\nrenderImGui"]
        IServerGame["IServerGame\ninitLogic\nupdate(dt)"]
    end

    subgraph gameImpl["Game — Implementations"]
        RtsGameClient["RtsGameClient\n(client-only Vulkan resources,\nmodels, terrain, menu, HUD)"]
        RtsGameServer["RtsGameServer\n(logic only)"]
        RtsGameBase["RtsGameBase\n(registry, spatialGrid, pathfinder,\nfogOfWar, lavaZone, networkManager)"]
    end

    subgraph sharedSystems["Shared ECS Systems"]
        direction LR
        OrderSys[OrderSystem]
        MoveSys[MovementSystem]
        CombatSys[CombatSystem]
        DeathSys[DeathSystem]
        ProjSim[ProjectileSim]
        LavaSim[LavaSim]
        FogSys[FogOfWar]
        Pathfinder[Pathfinder]
    end

    subgraph clientSystems["Client ECS Systems"]
        direction LR
        CameraSys[CameraSystem]
        SelectSys[SelectionSystem]
        InputSys[InputSystem]
        RenderSys[RenderSystem]
        FogRender[FogSystem]
        MinimapSys[MinimapSystem]
        HudSys[HudSystem]
    end

    CE -->|creates| RtsGameClient
    CE -->|creates & runs| Engine
    SE -->|creates| RtsGameServer
    SE -->|creates & runs| HeadlessRunner

    Engine -->|"run(IGame &)"| IGame
    HeadlessRunner -->|"run(IServerGame &)"| IServerGame

    RtsGameClient -.->|implements| IGame
    RtsGameServer -.->|implements| IServerGame
    RtsGameClient -->|extends| RtsGameBase
    RtsGameServer -->|extends| RtsGameBase

    RtsGameBase --> sharedSystems
    RtsGameClient --> clientSystems
```

---

## 2. Vulkan Stack (Engine-owned objects)

Init order from top to bottom. `ResourceContext` bundles non-owning refs passed to `IGame::initGraphics`.

```mermaid
graph TD
    Window -->|GLFW extensions| Instance
    Instance --> ValidationLayers
    Instance --> Surface
    Surface --> PhysicalDevice
    PhysicalDevice --> LogicalDevice
    LogicalDevice --> SwapChain
    SwapChain --> DepthBuffer
    DepthBuffer --> GraphicsPipeline
    GraphicsPipeline --> Renderer
    GraphicsPipeline --> UniformBuffer

    RC["ResourceContext\n(non-owning refs)"]
    LogicalDevice -.-> RC
    GraphicsPipeline -.-> RC
    Renderer -.->|commandPool| RC
    SwapChain -.-> RC
    Window -.-> RC
    RC -->|passed to| IGame

    style RC fill:#f5f0e8,stroke:#888
```

---

## 3. Sequence — Engine Frame Loop (client)

One iteration of the `Engine::run` while-loop.

```mermaid
sequenceDiagram
    participant W as Window
    participant E as Engine
    participant G as RtsGameClient
    participant UB as UniformBuffer
    participant R as Renderer

    loop every frame
        W->>E: pollEvents()
        E->>G: update(dt, extent)
        note over G: run ECS systems:<br/>InputSystem → OrderSystem → MovementSystem<br/>CombatSystem → DeathSystem → ProjectileSim<br/>LavaSim → CameraSystem → RenderSystem
        G-->>E: FrameOutput {draws, view, proj}
        E->>UB: update(view, proj)
        E->>R: drawFrame(draws, uniformBuffer, depthBuffer, ...)
        R->>G: renderImGui(cmd)
        note over G: HudSystem, MinimapSystem,<br/>FogSystem, MenuSystem
        R-->>E: needsResize?
        alt swap chain out of date
            E->>E: recreateSwapChain()
            E->>G: onSwapChainRecreated(swapChain)
        end
    end
```

---

## 4. Sequence — Network: connect, snapshot, and input

Client connects to server, receives unit assignment, then exchanges snapshots and input each tick.

```mermaid
sequenceDiagram
    participant C as RtsGameClient
    participant NMC as NetworkManager (client)
    participant NMS as NetworkManager (server)
    participant S as RtsGameServer

    C->>NMC: connectToServer(host, port)
    NMC-->>NMS: ENet connect handshake
    NMS->>S: onConnect(peer)
    S->>S: spawnUnit(position, Player)
    S->>S: assign NetworkId
    S->>NMS: sendReliableTo(peer, PlayerAssignment)
    NMS-->>NMC: reliable packet
    NMC->>C: onPacket → clientHandleAssignment()
    note over C: myNetworkId set, faction set

    loop every server tick (~60 Hz)
        S->>S: update(dt)
        note over S: OrderSystem → MovementSystem<br/>CombatSystem → DeathSystem<br/>ProjectileSim → LavaSim
        S->>S: snapshotTimer += dt
        alt snapshotTimer >= 0.05s (20 Hz)
            S->>S: serverSendSnapshot()
            S->>NMS: broadcastUnreliable(snapshot)
            NMS-->>NMC: unreliable packet
            NMC->>C: onPacket → clientApplySnapshot()
            note over C: update entity positions,<br/>health, lava radius from snapshot
        end
    end

    note over C: player right-clicks or presses Q
    C->>NMC: sendToServerUnreliable(InputPacket)
    NMC-->>NMS: unreliable packet
    NMS->>S: onPacket → serverHandleInput()
    S->>S: look up entity via peerToNetId_
    S->>S: enqueue MoveOrder / AbilityOrder
```
