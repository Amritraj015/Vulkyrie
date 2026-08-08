#pragma once

namespace Vulkyrie {

    struct Message {
        enum class Type { Error = 1, Warning = 2, Information = 4 };

        /** @brief The message text. */
        std::string text;

        /** @brief The type of message. */
        Type type;

        /** @brief Creates a new message.
         * @param text The raw text for the message.
         * @param type The type of message (Default - Error).
         */
        Message(std::string text, Type type = Type::Error)
            : text(text)
            , type(type) {
        }
    };

} // namespace Vulkyrie
