# Phase2：Projectile 对象池

在 Project Settings → Game → MassLab Experiment 中，将 Backend Class 设为
**Pooled Actor Projectile Backend**。直接生成版本仍可选，用于性能对照。

## 生命周期

- `Prewarm`：提前创建指定类的空闲 Actor。数量表示所需的空闲容量，重复调用不会重复创建已经满足的容量。
- `SpawnBatch` → `AcquireProjectile`：优先从对应类的空闲数组尾部取出对象；池不足时创建新对象。每次使用更新 Owner、Transform、世界空间速度和 MaxDistance。
- 超出距离 → `ReleaseProjectile`：移出活跃集合，停止 Actor 和组件 Tick、碰撞、显示及运动，清理 Actor 计时器和旧释放委托，再放回对应类的空闲数组。
- `Reset`：把全部活跃弹丸回收到池内，保留实例和容量。
- `Shutdown`：解绑 EndPlay 回调，销毁活跃和空闲 Actor，清空池及世界引用。

空闲数组使用栈式取出，取出时不缩减数组容量。`UPROPERTY` 标记的集合负责向 GC 保留对象引用。
外部 `Destroy` 或 EndPlay 会同步移除池内记录。

## 预热和性能对照

在关卡中的 Projectile Spawner 上设置 **Projectile → Pool → Prewarm Count**。
默认 `0`，便于测量冷池；非零时会在 Spawner 的 BeginPlay 中同步预热。
也可以通过 Projectile Subsystem 的 `Prewarm Projectiles` 蓝图节点手动提前准备。
Direct Actor Backend 的预热操作为空，因此同一套 Spawner 配置可以用于两种后端。

预热不会消除创建成本：大量预热仍可能在准备阶段产生尖峰。此版本使用同步预热；分帧预热可以作为后续实验。
若要比较“纯复用”的发射成本，空闲容量应足够覆盖本次发射；持续发射时，还要考虑尚未回收的弹丸。

在 Unreal Insights 中可查看 `PoolBackend_Prewarm`、`PoolBackend_CreateProjectile`、
`PoolBackend_SpawnBatch`、`PoolBackend_ReleaseProjectile`、`PoolBackend_Reset`。
预热充分的发射阶段应不再出现 `PoolBackend_CreateProjectile`。

## 复用边界

这个池针对当前以 ProjectileMovementComponent 运动、按距离释放的 Projectile。
池按实际 Actor 类分组，保留蓝图根组件缩放；复用时恢复 UpdatedComponent，即使运动组件曾经 StopSimulating 也能重新运动。
Construction Script、BeginPlay 仅在首次创建时执行。蓝图中依赖每次发射重新启动的计时器、特效或其他自定义状态，需要加入每次使用的初始化逻辑。

池会保留历史需求形成的容量，直到 Shutdown；此版本不设置容量上限或自动缩容。
活跃弹丸的移动、碰撞及渲染成本仍然存在。

## 验证

自动化测试：`MassLab.Projectile.PoolLifecycle`、`MassLab.Projectile.PoolSubsystem`。
覆盖预热、GC 保留、同一 Actor 复用、运动停止后重启、缩放和 Owner 重置、
重复回收、类隔离、池不足增长、外部销毁、Reset 和 Shutdown。
