<!--
******************************************************************************
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/-->

# Introduction

This document defines roles in oneTBB project.

# Roles and responsibilities

oneTBB project defines three main roles:
 * [Contributor](#contributor)
 * [Code Owner](#code-Owner)
 * [Maintainer](#maintainer)

These roles are merit based. Refer to the corresponding section for specific
requirements and the nomination process.

## Contributor

A Contributor invests time and resources to improve oneTBB project.
Anyone can become a Contributor by bringing value in one of the following ways:
  * Answer questions from community members.
  * Submit feedback to design proposals.
  * Review and/or test pull requests.
  * Test releases and report bugs.
  * Contribute code, including bug fixes, features implementations,
and performance optimizations.
  * Contribute design proposals.

Responsibilities:
  * Follow the [Code of Conduct](CODE_OF_CONDUCT.md).
  * Follow the project [contributing guidelines](CONTRIBUTING.md).

Privileges:
  * Eligible to become Code Owner.

## Code Owner

A Code Owner has responsibility for a specific project component or a functional
area. Code Owners are collectively responsible, with other Code Owners,
for developing and maintaining their component or functional areas, including
reviewing all changes to corresponding areas of responsibility and indicating
whether those changes are ready to be merged. Code Owners have a track record of
contribution and review in the project.

Responsibilities:
  * Follow the [Code of Conduct](CODE_OF_CONDUCT.md).
  * Follow and enforce the project [contributing guidelines](CONTRIBUTING.md).
  * Co-own with other code owners a specific component or aspect of the library,
    including contributing bug fixes, implementing features, and performance
    optimizations.
  * Review pull requests in corresponding areas of responsibility.
  * Monitor testing results and flag issues in corresponding areas of
    responsibility.
  * Support and guide Contributors.

Requirements:
  * Track record of accepted code contributions to a specific project component.
  * Track record of contributions to the code review process.
  * Demonstrated in-depth knowledge of the architecture of a specific project
    component.
  * Commits to being responsible for that specific area.

Privileges:
  * PR approval counts towards approval requirements for a specific component.
  * Can promote fully approved Pull Requests to the `master` branch.
  * Can recommend Contributors to become Code Owners.
  * Eligible to become a Maintainer.

The process of becoming a Code Owner is:
1. A Contributor is nominated by opening a PR modifying the MAINTAINERS.md file
including name, Github username, and affiliation.
2. At least two specific component Maintainers approve the PR.
3. CODEOWNERS.md file is updated to represent corresponding areas of responsibility.

## Maintainer
Maintainers are the most established contributors who are responsible for the 
project technical direction and participate in making decisions about the
strategy and priorities of the project.

Responsibilities:
  * Follow the [Code of Conduct](CODE_OF_CONDUCT.md).
  * Follow and enforce the project [contributing guidelines](CONTRIBUTING.md)
  * Co-own with other component Maintainers on the technical direction of a specific component.
  * Co-own with other Maintainers on the project as a whole, including determining strategy and policy for the project.
  * Suppport and guide Contributors and Code Owners.

Requirements:
  * Experience as a Code Owner for at least 12 months.
  * Track record of major project contributions to a specific project component.
  * Demonstrated deep knowledge of a specific project component.
  * Demonstrated broad knowledge of the project across multiple areas.
  * Commits to using priviledges responsibly for the good of the project.
  * Is able to exercise judgment for the good of the project, independent of
    their employer, friends, or team.

Privileges:
  * Can represent the project in public as a Maintainer.
  * Can promote Pull Requests to release branches and override mandatory
  checks when necessary.
  * Can recommend Code Owners to become Maintainers.

Process of becoming a maintainer:
1. A Maintainer may nominate a current code owner to become a new Maintainer by 
opening a PR against MAINTAINERS.md file.
2. A majority of the current Maintainers must then approve the PR.

# Code Owners and Maintainers List

## oneTBB core (API, Architecture, Tests)

| Name                  | Github ID             | Affiliation       | Role       |
| --------------------- | --------------------- | ----------------- | ---------- |
| Konstantin Boyarinov  | @kboyarinov           | Intel Corporation | ? |
| Aleksei Fedotov       | @aleksei-fedotov      | Intel Corporation | ? |
| Ilya Isaev            | @isaevil              | Intel Corporation | ? |
| Sarath Nandu R        | @sarathnandu          | Intel Corporation | ? |
| Dmitri Mokhov         | @dnmokhov             | Intel Corporation | ? |
| Michael Voss          | @vossmjp              | Intel Corporation | ? |
| Alexey Kukanov        | @akukanov             | Intel Corporation | ? |
| Pavel Kumbrasev       | @pavelkumbrasev       | Intel Corporation | ? |

## oneTBB TBBMALLOC (API, Architecture, Tests)

| Name                  | Github ID             | Affiliation       | Role       |
| --------------------- | --------------------- | ----------------- | ---------- |
| Łukasz Plewa          | @lplewa               | Intel Corporation | Maintainer |


## oneTBB Documentation

| Name                   | Github ID             | Affiliation       | Role       |
| ---------------------- | --------------------- | ----------------- | ---------- |
| Alexandra Epanchinzeva | @aepanchi             | Intel Corporation | Code Owner |


## oneTBB Release management

| Name               | Github ID             | Affiliation       | Role       |
| ------------------ | --------------------- | ----------------- | ---------- |
| Olga Malysheva     | @omalyshe             | Intel Corporation | Maintainer |

