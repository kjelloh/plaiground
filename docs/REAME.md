# plaiground

My agentic AI playground aiming and sandboxing AI code generation from production code

* [thinking](./thinking.md)
* [chimes](../chime/index.md)
* [Github Pages](https://kjelloh.github.io/plaiground/)

All documentation is licenced as CC0 1.0 Universal as defined in LICENCE file in top folder 'docs' - [Documentation Licence](./docs/LICENCE.txt)

All source code is licenced as GNU GENERAL PUBLIC LICENSE Version 2, June 1991 as defined in LICENCE file in top folder 'src' - [Source Code Licence](./src/LICENCE.txt)

## init_new and update_index

This is the current core of my documentation tooling and pipeline.

* init_new is a cross platform python script that creates a new 'document thing'

  * A 'document thing' is a 'type' and a header
  * E.g., for a 'chime' I can do:

```sh
>./init_new chime 'Consider this template chime?'
>./update_index chime
```

  * It will create a CAS (content adressable storage) folder structure:

  ```sh
  chime
├── 9ee24447
│   └── chime.md
└── index.md
  ```
  