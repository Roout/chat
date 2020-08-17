#ifndef INTERACTION_STAGE_HPP
#define INTERACTION_STAGE_HPP

namespace InteractionStage {
    /**
     * This enum defines stage which the interaction between client and server has already reached.
     */
    enum class State {
        /**
         * Client has just being created. Connection hasn't been initiated yet.
         * @note 
         * Session was created with already acccepted socket.
         */
        DISCONNECTED,
        /**
         * In this state session will only wait for AUTHORIZE request.
         * Any other requests will be ignored. Client in this state 
         * has to complete authorization.
         */
        UNAUTHORIZED,
        /**
         * In this state client already completed authorization and has access to 
         * chatrooms.
         */
        AUTHORIZED,
       
        COUNT
    };
}

namespace IStage = InteractionStage;

#endif // INTERACTION_STAGE_HPP