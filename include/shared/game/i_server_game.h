#ifndef I_SERVER_GAME_H
#define I_SERVER_GAME_H

namespace VulkanHelpers {

class IServerGame {
  public:
    virtual ~IServerGame() = default;
    virtual void initLogic() = 0;
    virtual void update(float dt) = 0;
    virtual bool wantsClose() const = 0;
};

} // namespace VulkanHelpers

#endif // I_SERVER_GAME_H
