FROM wiiuenv/devkitppc:20200810

COPY --from=devkitpro/devkitarm:20200730 $DEVKITPRO/devkitARM $DEVKITPRO/devkitARM

ENV DEVKITARM=/opt/devkitpro/devkitARM

WORKDIR project