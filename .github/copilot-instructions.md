All code must compile without errors.
All code must follow best practices for performance optimization.
All code must follow best practices for security.
All code must follow best practices for user experience.
All code must be based on real-life results and scenarios - no hypothetical or simulated situations.
All code must be well-documented with comments explaining the purpose and functionality of each section.
All code must be written in English.
All code must avoid directly accessing resources that other threads are already using. Use proper synchronization techniques to manage concurrent access to shared resources.
All code must avoid race conditions.
All code must have proper error handling and logging mechanisms to capture and log exceptions without crashing the application.
All code must be modular and follow the Single Responsibility Principle, ensuring that each class or method has a single purpose.
All code must have a consistent coding style and naming conventions throughout the codebase.
All code must have a purpose. Unreachable code or unused functions must be removed, so long as it doesn't break the application.
All code must ensure that long running processes or loops do not run on the parent thread, which could cause the application to stop responding or perform poorly.
All code must properly handle null values to prevent null reference exceptions.
All code must provide values for required modifiers and parameters, or make the parameters optional/nullable.
All code must avoid using deprecated or obsolete APIs and libraries. Use the latest stable versions available.
You must compile the solution when you're done making changes. Should it fail to build, you must fix all errors until the build is successful.
Always assume there is more than one culprit to any given problem you're asked to solve. Look for all possible issues, not just the most obvious one.
All code must be tested to ensure it works as intended and does not introduce new issues or bugs.
All code must not use hard-coded values that should otherwise be stored in a configuration file. All code must use configuration files for such values.
Your responses should be broken down in pieces to avoid truncation. If your response is cut off, I will prompt you to continue.
All code that runs on the main thread must avoid using timers, which can cause the UI to be sluggish or unresponsive.

Break up work to keep the token count under 64,000 to avoid errors and truncations.

