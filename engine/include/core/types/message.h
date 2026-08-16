#pragma once

namespace Vulkyrie {

    struct Message {
        enum class MessageType : u8 { Error = 1, Warning = 2, Information = 4 };

        /** @brief The message text. */
        std::string Text;

        /** @brief The type of message. */
        MessageType Type;

        /** @brief Creates a new message.
         * @param text The raw text for the message.
         * @param type The type of message (Default - Error).
         */
        Message(std::string text, MessageType type = MessageType::Error)
            : Text(text)
            , Type(type) {
        }
    };

} // namespace Vulkyrie
