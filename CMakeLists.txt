cmake_minimum_required(VERSION 3.20)
project(BallBattle VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL Core Gui Sql WebSockets Charts)

set(SOURCES
    src/main.cpp

    src/app/GameApp.cpp
    src/app/MainWindow.cpp

    src/engine/GameEngine.cpp
    src/engine/GameState.cpp
    src/engine/World.cpp
    src/engine/IController.cpp
    src/engine/WorldSnapshot.cpp

    src/network/ExternalAgentServer.cpp
    src/network/NetworkClient.cpp

    src/entity/Food.cpp
    src/entity/Player.cpp
    src/entity/Spore.cpp
    src/entity/Virus.cpp
    src/entity/BigBean.cpp

    src/physics/PhysicsEngine.cpp

    src/ai/AIController.cpp

    src/audio/AudioManager.cpp

    src/storage/DatabaseManager.cpp

    src/record/RecordManager.cpp

    src/util/Math.cpp
    src/util/Config.cpp
    src/util/Random.cpp

    src/renderer/GLWidget.cpp
    src/renderer/GLRenderer.cpp
    src/renderer/Camera.cpp
    src/renderer/ShaderManager.cpp
    src/renderer/GridRenderer.cpp
    src/renderer/BallRenderer.cpp
    src/renderer/FoodRenderer.cpp
    src/renderer/VirusRenderer.cpp
    src/renderer/SporeRenderer.cpp
    src/renderer/BigBeanRenderer.cpp
    src/renderer/QPainterGLWidget.cpp
    src/renderer/MinimapRenderer.cpp

    src/particle/ParticleSystem.cpp

    src/skin/SkinManager.cpp

    src/ui/HUDOverlay.cpp
    src/ui/HUDSettingsPanel.cpp
    src/ui/VirtualJoystick.cpp
    src/ui/MainMenu.cpp
    src/ui/SubWindow.cpp
    src/ui/SubWindowManager.cpp
    src/ui/SettingsWindow.cpp
    src/ui/RecordsWindow.cpp
    src/ui/RankWindow.cpp

    src/ranking/RankSystem.cpp
    src/ranking/SeasonManager.cpp

    src/achievement/AchievementManager.cpp

    src/ui/AchievementWindow.cpp
    src/ui/GameOverScreen.cpp
    src/ui/LeaderboardWidget.cpp
    src/ui/ModeSelectWindow.cpp
    src/ui/DebugPanel.cpp
    src/ui/Style.cpp
)

set(HEADERS
    src/app/GameApp.h
    src/app/MainWindow.h

    src/engine/GameEngine.h
    src/engine/GameState.h
    src/engine/World.h
    src/engine/IController.h
    src/engine/HumanController.h
    src/engine/WorldSnapshot.h

    src/network/ExternalAgentServer.h

    src/entity/Entity.h
    src/entity/Food.h
    src/entity/Player.h
    src/entity/Spawner.h
    src/entity/Spore.h
    src/entity/Virus.h
    src/entity/BigBean.h

    src/physics/PhysicsEngine.h

    src/ai/Behaviors.h
    src/ai/AIController.h

    src/audio/AudioManager.h

    src/storage/Schema.h
    src/storage/DatabaseManager.h

    src/record/GameRecord.h
    src/record/RecordManager.h

    src/util/Math.h
    src/util/Config.h
    src/util/Random.h

    src/renderer/GLWidget.h
    src/renderer/GLRenderer.h
    src/renderer/Camera.h
    src/renderer/ShaderManager.h
    src/renderer/GridRenderer.h
    src/renderer/BallRenderer.h
    src/renderer/FoodRenderer.h
    src/renderer/VirusRenderer.h
    src/renderer/SporeRenderer.h
    src/renderer/BigBeanRenderer.h
    src/renderer/QPainterGLWidget.h
    src/renderer/MinimapRenderer.h

    src/particle/Particle.h
    src/particle/ParticleSystem.h

    src/skin/SkinManager.h

    src/ui/HUDOverlay.h
    src/ui/HUDSettingsPanel.h
    src/ui/VirtualJoystick.h
    src/ui/MainMenu.h
    src/ui/SubWindow.h
    src/ui/SubWindowManager.h
    src/ui/SettingsWindow.h
    src/ui/RecordsWindow.h
    src/ui/RankWindow.h

    src/ranking/RankSystem.h
    src/ranking/SeasonManager.h

    src/achievement/AchievementDef.h
    src/achievement/AchievementManager.h

    src/ui/AchievementWindow.h
    src/ui/GameOverScreen.h
    src/ui/ModeSelectWindow.h
    src/ui/DebugPanel.h
    src/ui/Style.h
)

qt_add_executable(BallBattle WIN32 ${SOURCES} ${HEADERS})

target_include_directories(BallBattle PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(BallBattle PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
    Qt6::WebSockets
    Qt6::Charts
    
    ${CMAKE_DL_LIBS}
    opengl32
    gdi32
    winmm
)

file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})
