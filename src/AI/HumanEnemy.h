//
// Created by engin on 27.11.2017.
//

#ifndef LIMONENGINE_HUMANENEMY_H
#define LIMONENGINE_HUMANENEMY_H


#include "Actor.h"
#include "../Utils/GLMUtils.h"

class HumanEnemy: public Actor {
    const long PLAYER_SHOOT_TIMEOUT = 1000;
    long playerPursuitStartTime = 0L;
    long playerPursuitTimeout = 500000L; //if not see player for this amount, return.
    bool returnToPosition = false;
    glm::vec3 initialPosition;
    glm::vec3 lastWalkDirection;
    std::string currentAnimation;
    bool hitAnimationAwaiting = false;
    long dieAnimationStartTime = 0;
    long hitAnimationStartTime = 0;
    long lastSetupTime;
    long shootPlayerTimer = 0;
    uint32_t hitPoints = 100;

public:
    HumanEnemy(uint32_t id, LimonAPI *limonAPI) : Actor(id, limonAPI) {}

    void play(long time, ActorInformation &information, Options* options __attribute__((unused))) {//FIXME unused attribute is temporary
        lastSetupTime = time;

        if(hitPoints <= 0) {
            return;//if already dead, don't do anything
        }

        if(information.playerDead) {
            if(model->getAnimationName() !="idle|mixamo.com") {
                model->setAnimationWithBlend("idle|mixamo.com");
                playerPursuitStartTime = 0L;
            }
        }

        //first check if we just died
        if(hitPoints <= 0 && dieAnimationStartTime == 0) {
            model->setAnimationWithBlend("death from the front|mixamo.com", false);
            dieAnimationStartTime = time;
        }



        if(model->getAnimationName() == "Shoot Rifle|mixamo.com"  && model->isAnimationFinished()) {
            model->setAnimationWithBlend("run forward|mixamo.com");
        }

        //check if the player can be seen
        if(information.canSeePlayerDirectly && information.isPlayerFront && !information.playerDead) {
            if (playerPursuitStartTime == 0) {
                model->setAnimationWithBlend("run forward|mixamo.com");
                //means we will just start pursuit, mark the position so we can return.
                initialPosition = GLMConverter::BltToGLM(model->getRigidBody()->getCenterOfMassPosition());
                returnToPosition = true;
            }
            playerPursuitStartTime = time;
            if(shootPlayerTimer == 0) {
                shootPlayerTimer = time;
            } else {
                if(time - shootPlayerTimer > PLAYER_SHOOT_TIMEOUT) {
                    model->setAnimationWithBlend("Shoot Rifle|mixamo.com", false);
                    std::vector<LimonAPI::ParameterRequest> prList;
                    LimonAPI::ParameterRequest pr;
                    pr.valueType = pr.STRING;
                    strncpy(pr.value.stringValue, "SHOOT_PLAYER", 63);
                    prList.push_back(pr);
                    limonAPI->interactWithPlayer(prList);
                    shootPlayerTimer = time;
                    limonAPI->playSound("./Data/Sounds/shotgun.wav", this->getPosition(), false);
                }
            }
        }

        if(time - playerPursuitStartTime >= playerPursuitTimeout) {
            playerPursuitStartTime = 0;
        }

        if(playerPursuitStartTime == 0) {
            //if not in player pursuit
            if(returnToPosition) {
                //TODO search for route to initial position and return
            }
        } else {
            //if player pursuit mode

            //first check if we are hit
            if(hitAnimationAwaiting) {
                model->setAnimationWithBlend("Hit Reaction|mixamo.com", false);
                hitAnimationStartTime = time;
                hitAnimationAwaiting = false;//don't request another hit again
            }

            //now check if hit animation should be finished
            if(hitAnimationStartTime != 0 && time - hitAnimationStartTime > 500) { //play hit animation for 500 ms only
                model->setAnimationWithBlend("run forward|mixamo.com");
                initialPosition = GLMConverter::BltToGLM(model->getRigidBody()->getCenterOfMassPosition());
                returnToPosition = true;
                hitAnimationStartTime = 0;
            }

            if (information.canGoToPlayer) {
                //keep the last known direction, if player is at a unknown place.
                //FIXME this is a hack, normally this should not be necessary but sometimes even player is a valid place,
                //actor might not be for current implementation.
                lastWalkDirection = information.toPlayerRoute;
            }
            glm::vec3 moveDirection = 0.1f * lastWalkDirection;
            model->getTransformation()->addTranslate(moveDirection);
            if(information.isPlayerLeft) {
                if(information.cosineBetweenPlayerForSide < 0.95) {
                    glm::quat rotateLeft(1.0f, 0.0f, 0.015f, 0.0f);
                    model->getTransformation()->addOrientation(rotateLeft);
                }
            }
            if(information.isPlayerRight) {
                //turn just a little bit to right
                if(information.cosineBetweenPlayerForSide < 0.95) {
                    glm::quat rotateRight(1.0f, 0.0f, -0.015f, 0.0f);
                    model->getTransformation()->addOrientation(rotateRight);
                }
            }
            if(information.isPlayerUp) {
                //std::cout << "Up." << std::endl;
            }
            if(information.isPlayerDown) {
                //std::cout << "Down." << std::endl;
            }
        }
    }

    bool interaction(std::vector<LimonAPI::ParameterRequest> &interactionInformation) override {
        if(interactionInformation.size() < 1) {
            return false;
        }

        if(interactionInformation[0].valueType == LimonAPI::ParameterRequest::ValueTypes::STRING && std::string(interactionInformation[0].value.stringValue) == "GOT_HIT") {
            std::cout << "Enemy get hit!" << std::endl;
            playerPursuitStartTime = lastSetupTime;
            hitAnimationAwaiting = true;
            if(hitPoints < 20) {
                hitPoints =0;
            } else {
                hitPoints = hitPoints - 20;
            }
            return true;
        }
        return false;
    }

    virtual void IMGuiEditorView() {
        ImGui::Text("Human Enemy AI");
        if(playerPursuitStartTime == 0) {
            ImGui::Text("Status: Awaiting Player detection");
        } else {
            ImGui::Text("Status: Player pursuit");
            if(ImGui::Button("Stop pursuit")) {
                playerPursuitStartTime = 0;
                model->setAnimationWithBlend("idle|mixamo.com");
            }
        }

    }
};


#endif //LIMONENGINE_HUMANENEMY_H
