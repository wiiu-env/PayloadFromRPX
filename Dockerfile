FROM wiiuenv/devkitppc:20210414

COPY --from=devkitpro/devkitarm:20200730 $DEVKITPRO/devkitARM $DEVKITPRO/devkitARM

ENV DEVKITARM=/opt/devkitpro/devkitARM

WORKDIR project