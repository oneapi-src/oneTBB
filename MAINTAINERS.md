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

[permissions]: https://docs.github.com/en/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles/repository-roles-for-an-organization#permissions-for-each-role

|                                                                                                                                             |       Contributor       |       Code Owner        |       Maintainer        |
| :------------------------------------------------------------------------------------------------------------------------------------------ | :---------------------: | :---------------------: | :---------------------: |
| _Responsibilities_                                                                                                                          |                         |                         |                         |
| Follow the Code of Conduct                                                                                                                  |            ✓            |            ✓           |            ✓            |
| Follow Contribution Guidelines                                                                                                              |            ✓            |            ✓           |            ✓            |
| Enforce Contribution Guidelines                                                                                                             |            ✗            |            ✓           |            ✓            |
| Co-own component or aspect of the library,<br>  including contributing: bug fixes, implementing features,<br> and performance optimizations |            ✗            |            ✓           |            ✓            |
| Co-own on technical direction of component or<br> aspect of the library                                                                     |            ✗            |            ✗           |            ✓            |
| Co-own on the project as a whole,<br> including determining strategy and policy for the project                                             |            ✗            |            ✗           |            ✓            |
| _Privileges_                                                                                                                                |                         |                         |                         |
| Permission granted                                                                                                                          |   [Read][permissions]   |   [Write][permissions]  | [Maintain][permissions] |
| Eligible to become                                                                                                                          |       Code Owner        |       Maintainer        |            ✗            |
| Can recommend <span style="color:Green">Contributors<br> to become Code Owner                                                               |            ✗            |            ✓           |            ✓            |
| Can participate in promotions of<br> Code Owners and  Maintainers                                                                           |            ✗            |            ✓           |            ✓            |
| Can suggest Milestones during planning                                                                                                      |            ✓            |            ✓           |            ✓            |
| Can choose Milestones for specific component                                                                                                |            ✗            |            ✓           |            ✓            |
| Paticipate in project's Milestones planning                                                                                                 |            ✗            |            ✗           |            ✓            |
| Can propose new RFC or<br> participate in review of existing RFC                                                                            |            ✓            |            ✓           |            ✓            |
| Can request rework of RFCs<br> in represented area of responsibility                                                                        |            ✗            |            ✓           |            ✓            |
| Can request rework of RFCs<br> in any part of the project                                                                                   |            ✗            |            ✗           |            ✓            |
| Can manage release process of the project                                                                                                   |            ✗            |            ✗           |            ✓            |
| Can represent the project in public as a Maintainer                                                                                         |            ✗            |            ✗           |            ✓            |

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

## Code Owner

A Code Owner has responsibility for a specific project component or a functional
area. Code Owners are collectively responsible, with other Code Owners,
for developing and maintaining their component or functional areas, including
reviewing all changes to corresponding areas of responsibility and indicating
whether those changes are ready to be merged. Code Owners have a track record of
contribution and review in the project.

Requirements:
  * Track record of accepted code contributions to a specific project component.
  * Track record of contributions to the code review process.
  * Demonstrated in-depth knowledge of the architecture of a specific project
    component.
  * Commits to being responsible for that specific area.

The process of becoming a Code Owner is:
1. A Contributor is nominated by opening a PR modifying the MAINTAINERS.md file
including name, Github username, and affiliation.
2. At least two specific component Maintainers approve the PR.
3. CODEOWNERS.md file is updated to represent corresponding areas of responsibility.

## Maintainer
Maintainers are the most established contributors who are responsible for the 
project technical direction and participate in making decisions about the
strategy and priorities of the project.

Requirements:
  * Experience as a Code Owner.
  * Track record of major project contributions to a specific project component.
  * Demonstrated deep knowledge of a specific project component.
  * Demonstrated broad knowledge of the project across multiple areas.
  * Commits to using priviledges responsibly for the good of the project.
  * Is able to exercise judgment for the good of the project, independent of
    their employer, friends, or team.

Process of becoming a maintainer:
1. A Maintainer may nominate a current code owner to become a new Maintainer by 
opening a PR against MAINTAINERS.md file.
2. A majority of the current Maintainers must then approve the PR.

# Code Owners and Maintainers List

## oneTBB core (API, Architecture, Tests)

| Name                  | Github ID             | Affiliation       | Role       |
| --------------------- | --------------------- | ----------------- | ---------- |
| Konstantin Boyarinov  | @kboyarinov           | Intel Corporation | Code Owner |
| Aleksei Fedotov       | @aleksei-fedotov      | Intel Corporation | Code Owner |
| Ilya Isaev            | @isaevil              | Intel Corporation | Code Owner |
| Sarath Nandu R        | @sarathnandu          | Intel Corporation | Code Owner |
| Dmitri Mokhov         | @dnmokhov             | Intel Corporation | Code Owner |
| Michael Voss          | @vossmjp              | Intel Corporation | Maintainer |
| Alexey Kukanov        | @akukanov             | Intel Corporation | Code Owner |
| Pavel Kumbrasev       | @pavelkumbrasev       | Intel Corporation | Code Owner |

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

