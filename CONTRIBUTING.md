## Contributing to Melon

Hello, and welcome! Whether you are looking for help, trying to report a bug,
thinking about getting involved in the project, or about to submit a patch,
this document is for you!
It intends to be both an entry point for newcomers to the community (with various technical backgrounds), and a guide/reference for contributors and maintainers.

## Where to seek for help?

For questions around the use of the Melon, please use [GitHub Discussions](https://github.com/Water-Melon/Melon/discussions).

Please avoid opening GitHub issues for general questions or help, as those should be reserved for actual bug reports. The Melon community is welcoming and more than willing to assist you on those channels!

## Where to report bugs?

Feel free to [submit an issue](https://github.com/Water-Melon/Melon/issues) on the GitHub repository, we would be grateful to hear about it! Please make sure that you respect the GitHub issue template, and include:

1. A summary of the issue
2. A list of steps to help reproduce the issue
3. Your Melon configuration, or the parts that are relevant to your issue

If you wish, you are more than welcome to propose a patch to fix the issue! See the Submit a patch section for more information on how to best do so.

## Where to submit feature requests?

You can [submit an issue](https://github.com/Water-Melon/Melon/issues) for feature requests. Please make sure to add as much detail as you can when doing so.

You are also welcome to propose patches adding new features. See the section on Submitting a patch for details.

## Contributing

In addition to code enhancements and bug fixes, you can contribute by

1. Reporting a bug (see the report bugs section)
2. Helping other members of the community
3. Fixing a typo in the code
4. Providing your feedback on the proposed features and designs
5. Reviewing Pull Requests

If you wish to contribute code (features or bug fixes), see the Submitting a patch section.

## Submitting a patch

Feel free to contribute fixes or minor features by opening a Pull Request.
Small contributions are more likely to be merged quicker than changes which require a lot of time to review.
If you are planning to develop a larger feature, please talk to us first in the [GitHub Discussions](https://github.com/Water-Melon/Melon/discussions)!

When contributing, please follow the guidelines provided in this document.
They will cover topics such as the different Git branches we use, the commit message format to use, or the appropriate code style.

Once you have read them, and you feel that you are ready to submit your Pull Request, be sure to verify a few things:

1. Your commit history is clean: changes are atomic and the git message format was respected
2. Rebase your work on top of the base branch (seek help online on how to use git rebase; this is important to ensure your commit history is clean and linear)
3. The static linting is succeeding.

If the above guidelines are respected, your Pull Request has all its chances to be considered and will be reviewed by a maintainer.

If you are asked to update your patch by a reviewer, please do so!
Remember: you are responsible for pushing your patch forward.
If you contributed it, you are probably the one in need of it.
You must be ready to apply changes to it if necessary.

If your Pull Request was accepted and fixes a bug, adds functionality, or makes it significantly easier to use or understand Melon, congratulations!
You are now an official contributor to Melon.

Your change will be included in the subsequent release Changelog, and we will not forget to include your name if you are an external contributor. 😉

### Git branches

If you have write access to the GitHub repository, please follow the following naming scheme when pushing your branch(es):

- feat/foo-bar for new features
- fix/foo-bar for bug fixes
- refactor/foo-bar when refactoring code without any behavior change
- style/foo-bar when addressing some style issue
- docs/foo-bar for updates to the README.md, this file, or similar documents
- chore/foo-bar when the change does not concern the functional source
- perf/foo-bar for performance improvements

### Commit atomicity

When submitting patches, it is important that you organize your commits in logical units of work.
You are free to propose a patch with one or many commits, as long as their atomicity is respected.
This means that no unrelated changes should be included in a commit.

For example: you are writing a patch to fix a bug, but in your endeavour, you spot another bug.
Do not fix both bugs in the same commit!
Finish your work on the initial bug, propose your patch, and come back to the second bug later on.
This is also valid for unrelated style fixes, refactors, etc...

You should use your best judgment when facing such decisions.
A good approach for this is to put yourself in the shoes of the person who will review your patch:
will they understand your changes and reasoning just by reading your commit history?
Will they find unrelated changes in a particular commit?
They shouldn't!

Writing meaningful commit messages that follow our commit message format will also help you respect this mantra (see the below section).

### Commit message format

To maintain a healthy Git history, we ask of you that you write your commit messages as follows:

- The tense of your message must be present
- Your message must be prefixed by a type, and a scope
- The header of your message should not be longer than 50 characters
- A blank line should be included between the header and the body
- The body of your message should not contain lines longer than 72 characters

We strive to adapt the [conventional-commits](https://www.conventionalcommits.org/en/v1.0.0/) format.

Here is a template of what your commit message should look like:

```
<type>(<scope>): <subject>
<BLANK LINE>
<body>
<BLANK LINE>
<footer>
```

#### Type

The type of your commit indicates what type of change this commit is about. The accepted types are:

- **feat**: A new feature
- **fix**: A bug fix
- **docs**: Changes to the README.md, this file, or other such documents
- **style**: Changes that do not affect the meaning of the code (white-space trimming, formatting, etc...)
- **perf**: A code change that significantly improves performance
- **refactor**: A code change that neither fixes a bug nor adds a feature, and is too big to be considered just perf
- **chore**: Maintenance changes related to code cleaning that isn't considered part of a refactor, build process updates, dependency bumps, or auxiliary tools and libraries updates

#### Scope

The scope is the part of the codebase that is affected by your change.
Choosing it is at your discretion, but here are some of the most frequent ones:

- **framework**: A change that affects the frameworks
- **deps**: When updating or adding dependencies (to be used with the `chore` prefix)
- **conf**: Configuration-related changes (new directives, improvements...)
- `<module-name>`: This could be `array`, or `rbtree` for example
- `*`: When the change affects too many parts of the codebase at once (this should be rare and avoided)

#### Subject

Your subject should contain a succinct description of the change. It should be written so that:

- It uses the present, imperative tense: "fix typo", and not "fixed" or "fixes"
- It is not capitalized: "fix typo", and not "Fix typo"
- It does not include a period. 😄

#### Body

The body of your commit message should contain a detailed description of your changes.
Ideally, if the change is significant, you should explain its motivation, the chosen implementation, and justify it.

As previously mentioned, lines in the commit messages should not exceed 72 characters.

#### Footer

The footer is the ideal place to link to related material about the change: related GitHub issues, Pull Requests, fixed bug reports, etc...

#### Examples
Here are a few examples of good commit messages to take inspiration from:

```
docs(json): add json parse section

Add a section of function `mln_json_parse` to the JSON document.
```

Or:

```
refactor(configure): refactor configure to be more readable and clear

Use shell functions to encapsulate various functions in the configure script, making the configure logic clearer, more readable and maintainable.
```

## Code style

In order to ensure a healthy and consistent codebase, we ask of you that you respect the adopted code style.
This section contains a non-exhaustive list of preferred styles for writing C.

- No line should be longer than 80 characters
- Indentation should consist of 4 spaces
- Each externed function should be named with a prefix `mln_` and the rest part should be related to the current module, such `mln_rbtree_xxx`
  
When you are unsure about the style to adopt, please browse other parts of the codebase to find a similar case, and stay consistent with it.

You might also notice places in the codebase where the described style is not respected. This is due to legacy code. **Contributions to update the code to the recommended style are welcome!**
